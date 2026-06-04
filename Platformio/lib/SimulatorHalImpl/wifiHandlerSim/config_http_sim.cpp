#if defined(IS_SIMULATOR)

#include "config_http.hpp"
#include "config_reload.hpp"
#include "editor_sync_mode.hpp"
#include "http_sim_util.hpp"

#include "Hardware/wifiHandlerInterface.h"
#include "HardwareFactory.hpp"
#include "RapidJsonUtilty.hpp"
#include "device_settings.hpp"
#include "device_settings_schema.hpp"

#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

#ifndef FS_PATH
#define FS_PATH "./sim_data/"
#endif

#ifndef CONFIG_HTTP_SIM_PORT
#define CONFIG_HTTP_SIM_PORT 9080
#endif

namespace {

using HandlerFn = void (*)(const std::string &method, const std::string &path,
                           const std::map<std::string, std::string> &query, const std::string &body);

SOCKET gListenFd = INVALID_SOCKET;
bool gRunning = false;
std::string gMdnsHost = "omote-sim";

int gPendingStatus = 200;
std::string gPendingBody;
const char *gPendingType = "application/json";

struct SimIrCapture {
  bool valid = false;
  std::string protocol;
  std::string dataHex;
  std::string human;
} gIrCapture;

std::string vfsPath(const std::string &rel) {
  std::string base = FS_PATH;
  if (!base.empty() && base.back() == '/')
    base.pop_back();
  return base + "/" + rel;
}

bool isSafePath(const std::string &path) {
  return !path.empty() && path[0] != '/' && path.find("..") == std::string::npos;
}

void flushResponse(SOCKET client) {
  http_sim::sendResponse(client, gPendingStatus, gPendingStatus == 204 ? "No Content" : "OK", gPendingType, gPendingBody);
}

void sendJsonResponse(SOCKET client, int code, const std::string &body) {
  http_sim::sendResponse(client, code, code >= 400 ? "Error" : "OK", "application/json", body);
}

void collectJsonFiles(const std::filesystem::path &root, const std::string &prefix, std::vector<std::string> &out) {
  if (!std::filesystem::exists(root))
    return;
  for (const auto &entry : std::filesystem::directory_iterator(root)) {
    const auto name = entry.path().filename().string();
    const std::string rel = prefix.empty() ? name : prefix + "/" + name;
    if (entry.is_directory()) {
      if (rel == "editor")
        continue;
      collectJsonFiles(entry.path(), rel, out);
    } else if (entry.is_regular_file()) {
      const auto ext = entry.path().extension().string();
      if (ext == ".json")
        out.push_back(rel);
    }
  }
}

/** Sim has no reboot: each /api/fs/write already marks the right config_reload flags. */
void simNotifyReload() {
  fflush(stdout);
  printf("[config_http_sim] Config applied in-process (sim stays running)\n");
  fflush(stdout);
}

void handleStatusImpl() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  auto wifi = HardwareFactory::getAbstract().wifi();
  const auto st = wifi ? wifi->GetStatus() : wifiHandlerInterface::wifiStatus{};
  d.AddMember("connected", st.isConnected, a);
  d.AddMember("ip", rapidjson::Value(st.IP.c_str(), a), a);
  d.AddMember("hostname", rapidjson::Value(gMdnsHost.c_str(), a), a);
  d.AddMember("api", "omote-config-v1", a);
  d.AddMember("editor_sync", editor_sync_mode::isActive(), a);
  gPendingBody = OMOTE::JSON::ToString(d);
}

void handleFsTree() {
  std::vector<std::string> files;
  collectJsonFiles(std::filesystem::path(FS_PATH), "", files);
  std::sort(files.begin(), files.end());
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  rapidjson::Value arr(rapidjson::kArrayType);
  for (const auto &f : files)
    arr.PushBack(rapidjson::Value(f.c_str(), a), a);
  d.AddMember("files", arr, a);
  gPendingBody = OMOTE::JSON::ToString(d);
}

void handleFsStat(const std::map<std::string, std::string> &query) {
  const auto it = query.find("path");
  if (it == query.end()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"missing path\"}";
    return;
  }
  const std::string &path = it->second;
  if (!isSafePath(path)) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid path\"}";
    return;
  }
  std::ifstream file(vfsPath(path), std::ios::binary);
  if (!file) {
    gPendingStatus = 404;
    gPendingBody = "{\"error\":\"not found\"}";
    return;
  }
  file.seekg(0, std::ios::end);
  const auto fileSize = static_cast<size_t>(file.tellg());
  file.close();
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("path", rapidjson::Value(path.c_str(), a), a);
  d.AddMember("size", static_cast<uint64_t>(fileSize), a);
  gPendingBody = OMOTE::JSON::ToString(d);
}

void handleFsReadRaw(const std::map<std::string, std::string> &query) {
  const auto it = query.find("path");
  if (it == query.end()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"missing path\"}";
    return;
  }
  const std::string &path = it->second;
  if (!isSafePath(path)) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid path\"}";
    return;
  }
  std::ifstream file(vfsPath(path), std::ios::binary);
  if (!file) {
    gPendingStatus = 404;
    gPendingBody = "{\"error\":\"not found\"}";
    return;
  }
  gPendingBody.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  gPendingType = "application/json; charset=utf-8";
}

void handleFsRead(const std::map<std::string, std::string> &query) {
  const auto it = query.find("path");
  if (it == query.end()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"missing path\"}";
    return;
  }
  const std::string &path = it->second;
  if (!isSafePath(path)) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid path\"}";
    return;
  }
  std::ifstream file(vfsPath(path));
  if (!file) {
    gPendingStatus = 404;
    gPendingBody = "{\"error\":\"not found\"}";
    return;
  }
  file.seekg(0, std::ios::end);
  const auto fileSize = static_cast<size_t>(file.tellg());
  if (fileSize > 96 * 1024) {
    gPendingStatus = 413;
    gPendingBody = "{\"error\":\"file too large\"}";
    return;
  }
  file.seekg(0, std::ios::beg);

  std::string content;
  content.reserve(fileSize);
  content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());

  rapidjson::StringBuffer buff;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
  writer.StartObject();
  writer.Key("path");
  writer.String(path.c_str(), static_cast<rapidjson::SizeType>(path.size()));
  writer.Key("content");
  writer.String(content.c_str(), static_cast<rapidjson::SizeType>(content.size()));
  writer.EndObject();
  gPendingBody.assign(buff.GetString(), buff.GetSize());
}

/** Per-file marks for HA/device settings; scene/page hot-reload is batched on reboot (see handleReboot). */
void markPathDirty(const std::string &path) {
  if (path == "HaSettings.json")
    config_reload::markHaSettingsDirty();
  else if (path == "DeviceSettings.json")
    config_reload::markDeviceSettingsDirty();
  else if (path == "DeviceSettings.schema.json")
    config_reload::markDeviceSettingsSchemaDirty();
}

void handleFsDelete(const std::map<std::string, std::string> &query) {
  const auto it = query.find("path");
  if (it == query.end()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"missing path\"}";
    return;
  }
  const std::string &path = it->second;
  if (!isSafePath(path)) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid path\"}";
    return;
  }
  const auto full = vfsPath(path);
  if (!std::filesystem::exists(full)) {
    gPendingStatus = 404;
    gPendingBody = "{\"error\":\"not found\"}";
    return;
  }
  std::error_code ec;
  if (!std::filesystem::remove(full, ec)) {
    gPendingStatus = 500;
    gPendingBody = "{\"error\":\"delete failed\"}";
    return;
  }
  markPathDirty(path);
  gPendingBody = "{\"ok\":true}";
}

void handleFsWrite(const std::map<std::string, std::string> &query, const std::string &body) {
  const auto it = query.find("path");
  if (it == query.end()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"missing path\"}";
    return;
  }
  const std::string &path = it->second;
  if (!isSafePath(path)) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid path\"}";
    return;
  }
  const std::filesystem::path full(vfsPath(path));
  std::error_code ec;
  if (const auto parent = full.parent_path(); !parent.empty())
    std::filesystem::create_directories(parent, ec);
  std::ofstream file(full, std::ios::out | std::ios::trunc);
  if (!file) {
    gPendingStatus = 500;
    gPendingBody = "{\"error\":\"write failed\"}";
    return;
  }
  file << body;
  markPathDirty(path);
  if (path == "Scenes.json" || path.rfind("Pages/", 0) == 0 || path.rfind("Scenes/", 0) == 0) {
    printf("[config_http_sim] wrote %s (%zu bytes)\n", path.c_str(), body.size());
    fflush(stdout);
  }
  gPendingBody = "{\"ok\":true}";
}

void handleDeviceSettingsSchemaGet() {
  if (!device_settings_schema::loadFromLittleFS()) {
    gPendingStatus = 404;
    gPendingBody = "{\"error\":\"schema not found\"}";
    return;
  }
  gPendingBody = OMOTE::JSON::ToString(device_settings_schema::document());
}

void handleDeviceSettingsGet() {
  rapidjson::Document d = device_settings::toJsonDocument();
  auto &a = d.GetAllocator();
  const auto &s = device_settings::currentConst();
  d.AddMember("sleep_timeout_ms", rapidjson::Value(static_cast<uint64_t>(s.displayTimeoutMs)), a);
  d.AddMember("display_off", device_settings::isScreenPoweredOff(), a);
  auto wifi = HardwareFactory::getAbstract().wifi();
  const auto st = wifi ? wifi->GetStatus() : wifiHandlerInterface::wifiStatus{};
  d.AddMember("wifi_connected", st.isConnected, a);
  if (st.isConnected) {
    d.AddMember("wifi_ssid", rapidjson::Value(st.ssid.c_str(), a), a);
    d.AddMember("ip", rapidjson::Value(st.IP.c_str(), a), a);
  }
  gPendingBody = OMOTE::JSON::ToString(d);
}

void handleDeviceSettingsPost(const std::string &body) {
  if (body.empty()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"missing body\"}";
    return;
  }
  rapidjson::Document doc;
  if (doc.Parse(body.c_str()).HasParseError() || !doc.IsObject()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid json\"}";
    return;
  }
  if (!device_settings::mergeFromJson(doc)) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid json\"}";
    return;
  }
  device_settings::applyToHardware();
  device_settings::saveToLittleFS();
  HardwareFactory::getAbstract().saveSettings();
  config_reload::markDeviceSettingsDirty();
  gPendingBody = "{\"ok\":true}";
}

void handleReboot() {
  printf("[config_http_sim] POST /api/device/reboot\n");
  fflush(stdout);
  editor_sync_mode::exit(false);
  config_reload::markPagesDirty();
  simNotifyReload();
  gPendingBody = "{\"ok\":true,\"restart\":true}";
}

void handleEditorSyncGet() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("editor_sync", editor_sync_mode::isActive(), a);
  gPendingBody = OMOTE::JSON::ToString(d);
}

void handleEditorSyncPost(const std::string &body) {
  rapidjson::Document d;
  if (body.empty() || d.Parse(body.c_str()).HasParseError()) {
    gPendingStatus = 400;
    gPendingBody = "{\"error\":\"invalid json\"}";
    return;
  }
  const bool on = d.HasMember("on") && d["on"].IsBool() && d["on"].GetBool();
  bool reboot = true;
  if (d.HasMember("reboot") && d["reboot"].IsBool())
    reboot = d["reboot"].GetBool();
  if (on) {
    editor_sync_mode::enter();
    gPendingBody = "{\"ok\":true,\"editor_sync\":true}";
  } else {
    editor_sync_mode::exit(false);
    if (reboot) {
      config_reload::markPagesDirty();
      simNotifyReload();
    }
    gPendingBody = reboot ? "{\"ok\":true,\"editor_sync\":false,\"restart\":true}"
                          : "{\"ok\":true,\"editor_sync\":false}";
  }
}

void handleIrLearnStart() {
  gIrCapture = {};
  if (auto ir = HardwareFactory::getAbstract().ir())
    ir->enableRx();
  gPendingBody = "{\"ok\":true}";
}

void handleIrLearnStop() {
  if (auto ir = HardwareFactory::getAbstract().ir())
    ir->disableRx();
  gPendingBody = "{\"ok\":true}";
}

void handleIrLearnPoll() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  if (gIrCapture.valid) {
    d.AddMember("ok", true, a);
    d.AddMember("protocol", rapidjson::Value(gIrCapture.protocol.c_str(), a), a);
    d.AddMember("code", rapidjson::Value(gIrCapture.dataHex.c_str(), a), a);
    d.AddMember("human", rapidjson::Value(gIrCapture.human.c_str(), a), a);
  } else {
    d.AddMember("ok", false, a);
  }
  gPendingBody = OMOTE::JSON::ToString(d);
}

void dispatch(SOCKET client, const std::string &method, const std::string &path,
              const std::map<std::string, std::string> &query, const std::string &body) {
  gPendingStatus = 200;
  gPendingBody = "{\"error\":\"not found\"}";
  gPendingType = "application/json";

  if (method == "OPTIONS") {
    gPendingStatus = 204;
    gPendingBody.clear();
    flushResponse(client);
    return;
  }

  if (method == "GET" && path == "/api/status") {
    handleStatusImpl();
  } else if (method == "GET" && path == "/api/fs/tree") {
    handleFsTree();
  } else if (method == "GET" && path == "/api/fs/stat") {
    handleFsStat(query);
  } else if (method == "GET" && path == "/api/fs/read/raw") {
    handleFsReadRaw(query);
  } else if (method == "GET" && path == "/api/fs/read") {
    handleFsRead(query);
  } else if ((method == "POST" || method == "PUT") && path == "/api/fs/write") {
    handleFsWrite(query, body);
  } else if (method == "POST" && path == "/api/fs/delete") {
    handleFsDelete(query);
  } else if (method == "GET" && path == "/api/device/settings/schema") {
    handleDeviceSettingsSchemaGet();
  } else if (method == "GET" && path == "/api/device/settings") {
    handleDeviceSettingsGet();
  } else if (method == "POST" && path == "/api/device/settings") {
    handleDeviceSettingsPost(body);
  } else if (method == "POST" && path == "/api/device/reboot") {
    handleReboot();
  } else if (method == "GET" && path == "/api/device/sync-mode") {
    handleEditorSyncGet();
  } else if (method == "POST" && path == "/api/device/sync-mode") {
    handleEditorSyncPost(body);
  } else if (method == "POST" && path == "/api/ir/learn/start") {
    handleIrLearnStart();
  } else if (method == "POST" && path == "/api/ir/learn/stop") {
    handleIrLearnStop();
  } else if (method == "GET" && path == "/api/ir/learn/poll") {
    handleIrLearnPoll();
  } else if (method == "GET" && path == "/") {
    gPendingBody = "{\"name\":\"OMOTE\",\"api\":\"/api/status\",\"sim\":true}";
  } else {
    gPendingStatus = 404;
  }

  flushResponse(client);
}

void handleClient(SOCKET client) {
  std::string method, path, body;
  std::map<std::string, std::string> query;
  if (!http_sim::recvRequest(client, method, path, query, body)) {
    closesocket(client);
    return;
  }
  dispatch(client, method, path, query, body);
  closesocket(client);
}

} // namespace

namespace config_http {

void begin(const char *mdnsName) {
  if (mdnsName && mdnsName[0])
    gMdnsHost = mdnsName;
  if (!http_sim::ensureSockets())
    return;
  if (gListenFd != INVALID_SOCKET)
    return;

  gListenFd = socket(AF_INET, SOCK_STREAM, 0);
  if (gListenFd == INVALID_SOCKET)
    return;

  int yes = 1;
  setsockopt(gListenFd, SOL_SOCKET, SO_REUSEADDR, reinterpret_cast<const char *>(&yes), sizeof(yes));

  sockaddr_in addr{};
  addr.sin_family = AF_INET;
  addr.sin_addr.s_addr = htonl(INADDR_LOOPBACK);
  addr.sin_port = htons(CONFIG_HTTP_SIM_PORT);
  if (bind(gListenFd, reinterpret_cast<sockaddr *>(&addr), sizeof(addr)) == SOCKET_ERROR) {
    closesocket(gListenFd);
    gListenFd = INVALID_SOCKET;
    return;
  }
  if (listen(gListenFd, 8) == SOCKET_ERROR) {
    closesocket(gListenFd);
    gListenFd = INVALID_SOCKET;
    return;
  }
  http_sim::setNonBlocking(gListenFd);
  gRunning = true;
  printf("[config_http_sim] Editor API: http://127.0.0.1:%u (Connect tab in config editor)\n",
         static_cast<unsigned>(CONFIG_HTTP_SIM_PORT));
}

void sync() {
  if (!gRunning || gListenFd == INVALID_SOCKET)
    return;

  for (int i = 0; i < 4; ++i) {
    sockaddr_in clientAddr{};
    socket_len_t len = sizeof(clientAddr);
    const SOCKET client = accept(gListenFd, reinterpret_cast<sockaddr *>(&clientAddr), &len);
    if (client == INVALID_SOCKET)
      break;
    http_sim::setNonBlocking(client);
    handleClient(client);
  }
}

void stop() {
  if (gListenFd != INVALID_SOCKET) {
    closesocket(gListenFd);
    gListenFd = INVALID_SOCKET;
  }
  gRunning = false;
}

bool isRunning() { return gRunning; }

} // namespace config_http

#endif // IS_SIMULATOR
