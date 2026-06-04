#if !defined(IS_SIMULATOR)
#include "config_http.hpp"
#include "config_reload.hpp"

#include "Hardware/LoggingInterface.hpp"
#include "HardwareFactory.hpp"
#include "RapidJsonUtilty.hpp"
#include "device_settings.hpp"
#include "device_settings_schema.hpp"
#include "display.hpp"
#include "editor_sync_mode.hpp"
#include "ir/IRTransceiver.hpp"
#include "LvglResourceManager.hpp"

#include <Arduino.h>
#include <lvgl.h>
#include <ESP.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

#include <algorithm>
#include <fstream>
#include <iterator>
#include <sstream>
#include <vector>

#include "rapidjson/stringbuffer.h"
#include "rapidjson/writer.h"

namespace {

#ifndef FS_PATH
#define FS_PATH "/littlefs/"
#endif

WebServer server(80);
bool running = false;
bool mdnsStarted = false;
std::string mdnsHost = "omote";
uint32_t rebootAtMs = 0;
uint32_t lastRemoteHttpMs = 0;

std::unique_ptr<LoggingInterface> logger;

void noteRemoteConfigActivity() {
  lastRemoteHttpMs = millis();
  // keepAwake blocks display sleep; do not wake/fade on every fs read (breaks touch/backlight).
}

/** HTTP handlers run on the main loop thread; yield + one LVGL tick so touch/UI stay responsive. */
void pumpUiDuringHttp() {
  yield();
  LvglResourceManager::GetInstance().AttemptNow([]() { lv_timer_handler(); });
}

IRTransceiver *irHw() {
  return static_cast<IRTransceiver *>(HardwareFactory::getAbstract().ir().get());
}

/** Copy into the document pool — never pass .c_str() from a temporary std::string or Arduino String. */
rapidjson::Value jsonCopy(const std::string &s, rapidjson::Document::AllocatorType &a) {
  return rapidjson::Value(s.c_str(), static_cast<rapidjson::SizeType>(s.size()), a);
}

std::string arduinoStringCopy(const String &s) {
  return std::string(s.c_str(), static_cast<size_t>(s.length()));
}

/** Max /api/fs/read JSON wrapper (legacy); prefer /api/fs/read/chunk. */
constexpr size_t kMaxFsReadBytes = 64 * 1024;
constexpr uint32_t kMinHeapReserve = 28000;
constexpr size_t kFsChunkMax = 8192;

size_t maxFsReadBytesForHeap() {
  const uint32_t freeHeap = ESP.getFreeHeap();
  if (freeHeap <= kMinHeapReserve + 4096)
    return 0;
  const size_t byHeap = static_cast<size_t>((freeHeap - kMinHeapReserve) / 2);
  return std::min(kMaxFsReadBytes, byHeap);
}

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(int code, const std::string &body) {
  noteRemoteConfigActivity();
  sendCors();
  server.sendHeader("Connection", "close");
  server.send(code, "application/json", body.c_str());
}

bool sendFsReadJson(const std::string &path, const std::string &content) {
  const size_t need = content.size() * 2 + path.size() + 128;
  if (ESP.getFreeHeap() < kMinHeapReserve + need) {
    sendJson(507, "{\"error\":\"insufficient memory\"}");
    return false;
  }

  rapidjson::StringBuffer buff;
  rapidjson::Writer<rapidjson::StringBuffer> writer(buff);
  writer.StartObject();
  writer.Key("path");
  writer.String(path.c_str(), static_cast<rapidjson::SizeType>(path.size()));
  writer.Key("content");
  writer.String(content.c_str(), static_cast<rapidjson::SizeType>(content.size()));
  writer.EndObject();

  const size_t len = buff.GetSize();
  if (!len) {
    sendJson(500, "{\"error\":\"serialize failed\"}");
    return false;
  }

  noteRemoteConfigActivity();
  sendCors();
  server.sendHeader("Connection", "close");
  // Use const char* send — avoids Arduino String copy that exhausts heap during bulk editor reads.
  server.send(200, "application/json", buff.GetString());
  pumpUiDuringHttp();
  return true;
}

bool isSafePath(const std::string &path) {
  if (path.empty() || path[0] == '/' || path.find("..") != std::string::npos)
    return false;
  return true;
}

std::string vfsPath(const std::string &rel) {
  std::string base = FS_PATH;
  if (!base.empty() && base.back() == '/')
    base.pop_back();
  return base + "/" + rel;
}

std::string lfsPath(const std::string &rel) { return "/" + rel; }

void collectFiles(const char *dirPath, std::vector<std::string> &out, const std::string &prefix) {
  File root = LittleFS.open(dirPath);
  if (!root || !root.isDirectory())
    return;

  uint32_t scanned = 0;
  File file = root.openNextFile();
  while (file) {
    std::string fullName = file.path() ? file.path() : file.name();
    std::string name = fullName;
    const auto slash = name.find_last_of('/');
    if (slash != std::string::npos)
      name = name.substr(slash + 1);

    std::string rel = prefix.empty() ? name : prefix + "/" + name;
    if (file.isDirectory()) {
      collectFiles(file.path(), out, rel);
    } else {
      out.push_back(rel);
    }
    file.close();
    file = root.openNextFile();
    if ((++scanned & 15u) == 0)
      pumpUiDuringHttp();
  }
  root.close();
}

void handleOptions() {
  noteRemoteConfigActivity();
  sendCors();
  server.send(204);
}

void handleStatus() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("connected", WiFi.isConnected(), a);
  if (WiFi.isConnected())
    d.AddMember("ip", jsonCopy(arduinoStringCopy(WiFi.localIP().toString()), a), a);
  else
    d.AddMember("ip", "", a);
  d.AddMember("hostname", jsonCopy(mdnsHost, a), a);
  d.AddMember("api", "omote-config-v1", a);
  d.AddMember("editor_sync", editor_sync_mode::isActive(), a);
  sendJson(200, OMOTE::JSON::ToString(d));
}

void handleFsTree() {
  std::vector<std::string> files;
  collectFiles("/", files, "");

  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  rapidjson::Value arr(rapidjson::kArrayType);
  for (const auto &f : files) {
    if (f.find(".json") != std::string::npos || f.find("editor/") == 0)
      arr.PushBack(jsonCopy(f, a), a);
  }
  std::sort(files.begin(), files.end());
  arr.Clear();
  for (const auto &f : files) {
    if (f.rfind("editor/", 0) == 0)
      continue;
    if (f.size() >= 5 && f.substr(f.size() - 5) == ".json")
      arr.PushBack(jsonCopy(f, a), a);
  }
  d.AddMember("files", arr, a);
  sendJson(200, OMOTE::JSON::ToString(d));
}

bool openFsPath(const std::string &path, File &out, size_t &outSize) {
  if (!isSafePath(path))
    return false;
  const std::string lfs = lfsPath(path);
  if (!LittleFS.exists(lfs.c_str()))
    return false;
  out = LittleFS.open(lfs.c_str(), "r");
  if (!out)
    return false;
  outSize = out.size();
  return true;
}

void handleFsStat() {
  noteRemoteConfigActivity();
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  File f;
  size_t sz = 0;
  if (!openFsPath(path, f, sz)) {
    sendJson(404, "{\"error\":\"not found\"}");
    return;
  }
  f.close();
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("path", jsonCopy(path, a), a);
  d.AddMember("size", static_cast<uint64_t>(sz), a);
  sendJson(200, OMOTE::JSON::ToString(d));
}

/** Stream file bytes — no JSON envelope (editor default on hardware). */
void handleFsReadRaw() {
  noteRemoteConfigActivity();
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  File f;
  size_t sz = 0;
  if (!openFsPath(path, f, sz)) {
    sendJson(404, "{\"error\":\"not found\"}");
    return;
  }
  if (sz > kMaxFsReadBytes) {
    f.close();
    sendJson(413, "{\"error\":\"file too large — use chunked read\"}");
    return;
  }
  pumpUiDuringHttp();
  sendCors();
  server.sendHeader("Connection", "close");
  server.sendHeader("X-OMOTE-Path", path.c_str());
  server.streamFile(f, "application/json; charset=utf-8");
  f.close();
  pumpUiDuringHttp();
}

/** Fixed-size chunk read — constant RAM (~2 KiB) for large config packs. */
void handleFsReadChunk() {
  noteRemoteConfigActivity();
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  File f;
  size_t total = 0;
  if (!openFsPath(path, f, total)) {
    sendJson(404, "{\"error\":\"not found\"}");
    return;
  }

  size_t offset = 0;
  if (server.hasArg("offset"))
    offset = static_cast<size_t>(strtoul(server.arg("offset").c_str(), nullptr, 10));
  size_t maxLen = kFsChunkMax;
  if (server.hasArg("max")) {
    maxLen = static_cast<size_t>(strtoul(server.arg("max").c_str(), nullptr, 10));
    if (maxLen == 0 || maxLen > kFsChunkMax)
      maxLen = kFsChunkMax;
  }
  if (offset > total) {
    f.close();
    sendJson(416, "{\"error\":\"offset past end\"}");
    return;
  }
  const size_t toRead = std::min(maxLen, total - offset);
  if (!f.seek(offset)) {
    f.close();
    sendJson(500, "{\"error\":\"seek failed\"}");
    return;
  }

  uint8_t buf[kFsChunkMax];
  const size_t n = f.read(buf, toRead);
  f.close();
  pumpUiDuringHttp();

  sendCors();
  server.sendHeader("Connection", "close");
  server.sendHeader("X-OMOTE-Path", path.c_str());
  server.sendHeader("X-OMOTE-Offset", String(static_cast<unsigned>(offset)));
  server.sendHeader("X-OMOTE-Total", String(static_cast<unsigned>(total)));
  server.sendHeader("X-OMOTE-More", (offset + n < total) ? "1" : "0");
  server.sendHeader("X-OMOTE-Length", String(static_cast<unsigned>(n)));
  server.setContentLength(n);
  server.send(200, "application/octet-stream", "");
  server.sendContent(reinterpret_cast<const char *>(buf), n);
  pumpUiDuringHttp();
}

void handleFsRead() {
  noteRemoteConfigActivity();
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  if (!isSafePath(path)) {
    sendJson(400, "{\"error\":\"invalid path\"}");
    return;
  }

  const size_t maxRead = maxFsReadBytesForHeap();
  if (!maxRead) {
    sendJson(507, "{\"error\":\"insufficient memory\"}");
    return;
  }

  std::ifstream file(vfsPath(path), std::ios::in);
  if (!file) {
    sendJson(404, "{\"error\":\"not found\"}");
    return;
  }
  file.seekg(0, std::ios::end);
  const auto fileSize = static_cast<size_t>(file.tellg());
  if (fileSize > maxRead) {
    sendJson(413, "{\"error\":\"file too large\"}");
    return;
  }
  file.seekg(0, std::ios::beg);

  std::string content;
  content.reserve(fileSize);
  content.assign(std::istreambuf_iterator<char>(file), std::istreambuf_iterator<char>());
  file.close();
  pumpUiDuringHttp();

  sendFsReadJson(path, content);
  pumpUiDuringHttp();
}

void handleFsDelete() {
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  if (!isSafePath(path)) {
    sendJson(400, "{\"error\":\"invalid path\"}");
    return;
  }

  const std::string full = vfsPath(path);
  if (!LittleFS.exists(lfsPath(path).c_str())) {
    sendJson(404, "{\"error\":\"not found\"}");
    return;
  }
  if (!LittleFS.remove(lfsPath(path).c_str())) {
    sendJson(500, "{\"error\":\"delete failed\"}");
    return;
  }
  if (path == "HaSettings.json")
    config_reload::markHaSettingsDirty();
  else if (path == "DeviceSettings.json")
    config_reload::markDeviceSettingsDirty();
  else if (path == "DeviceSettings.schema.json")
    config_reload::markDeviceSettingsSchemaDirty();
  sendJson(200, "{\"ok\":true}");
}

void handleFsWrite() {
  noteRemoteConfigActivity();
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  if (!isSafePath(path)) {
    sendJson(400, "{\"error\":\"invalid path\"}");
    return;
  }

  std::string body;
  if (server.hasArg("plain"))
    body = server.arg("plain").c_str();
  else if (server.hasArg("content"))
    body = server.arg("content").c_str();
  else if (server.hasArg("body"))
    body = server.arg("body").c_str();

  const std::string full = vfsPath(path);
  std::ofstream file(full, std::ios::out | std::ios::trunc);
  if (!file) {
    sendJson(500, "{\"error\":\"write failed\"}");
    return;
  }
  file << body;
  file.close();
  if (path == "HaSettings.json")
    config_reload::markHaSettingsDirty();
  else if (path == "DeviceSettings.json")
    config_reload::markDeviceSettingsDirty();
  else if (path == "DeviceSettings.schema.json")
    config_reload::markDeviceSettingsSchemaDirty();
  sendJson(200, "{\"ok\":true}");
}

void handleDeviceSettingsSchemaGet() {
  if (!device_settings_schema::loadFromLittleFS()) {
    sendJson(404, "{\"error\":\"schema not found\"}");
    return;
  }
  sendJson(200, OMOTE::JSON::ToString(device_settings_schema::document()));
}

void handleDeviceSettingsGet() {
  rapidjson::Document d = device_settings::toJsonDocument();
  auto &a = d.GetAllocator();
  const auto &s = device_settings::currentConst();
  d.AddMember("sleep_timeout_ms", rapidjson::Value(static_cast<uint64_t>(s.displayTimeoutMs)), a);
  d.AddMember("display_off", device_settings::isScreenPoweredOff(), a);
  d.AddMember("wifi_connected", WiFi.isConnected(), a);
  if (WiFi.isConnected()) {
    d.AddMember("wifi_ssid", jsonCopy(arduinoStringCopy(WiFi.SSID()), a), a);
    d.AddMember("ip", jsonCopy(arduinoStringCopy(WiFi.localIP().toString()), a), a);
  }
  sendJson(200, OMOTE::JSON::ToString(d));
}

void handleDeviceSettingsPost() {
  if (!server.hasArg("plain") && !server.hasArg("body")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }
  const std::string body =
      server.hasArg("plain") ? server.arg("plain").c_str() : server.arg("body").c_str();
  rapidjson::Document doc;
  if (doc.Parse(body.c_str()).HasParseError() || !doc.IsObject()) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }
  if (!device_settings::mergeFromJson(doc)) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }
  device_settings::applyToHardware();
  device_settings::saveToLittleFS();
  HardwareFactory::getAbstract().saveSettings();
  config_reload::markDeviceSettingsDirty();
  sendJson(200, "{\"ok\":true}");
}

void prepareForRestart() {
  device_settings::notifyActivity();
  if (auto disp = std::static_pointer_cast<Display>(HardwareFactory::getAbstract().display()))
    disp->wake();
  rebootAtMs = millis() + 500;
}

void handleReboot() {
  prepareForRestart();
  sendJson(200, "{\"ok\":true,\"restart\":true}");
}

void handleEditorSyncGet() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("editor_sync", editor_sync_mode::isActive(), a);
  sendJson(200, OMOTE::JSON::ToString(d));
}

void handleEditorSyncPost() {
  if (!server.hasArg("plain") && !server.hasArg("body")) {
    sendJson(400, "{\"error\":\"missing body\"}");
    return;
  }
  std::string body = server.hasArg("plain") ? server.arg("plain").c_str() : server.arg("body").c_str();
  rapidjson::Document d;
  if (d.Parse(body.c_str()).HasParseError()) {
    sendJson(400, "{\"error\":\"invalid json\"}");
    return;
  }
  const bool on = d.HasMember("on") && d["on"].IsBool() && d["on"].GetBool();
  bool showOverlay = false;
  if (d.HasMember("show_overlay") && d["show_overlay"].IsBool())
    showOverlay = d["show_overlay"].GetBool();
  if (on) {
    editor_sync_mode::enter(showOverlay);
    sendJson(200, "{\"ok\":true,\"editor_sync\":true}");
  } else {
    bool reboot = true;
    if (d.HasMember("reboot") && d["reboot"].IsBool())
      reboot = d["reboot"].GetBool();
    editor_sync_mode::exit(reboot);
    if (reboot)
      prepareForRestart();
    sendJson(200, reboot ? "{\"ok\":true,\"editor_sync\":false,\"restart\":true}"
                        : "{\"ok\":true,\"editor_sync\":false}");
  }
}

void handleIrLearnStart() {
  auto *ir = irHw();
  if (!ir) {
    sendJson(500, "{\"error\":\"no ir\"}");
    return;
  }
  ir->clearLastCapture();
  ir->enableRx();
  sendJson(200, "{\"ok\":true}");
}

void handleIrLearnStop() {
  if (auto *ir = irHw())
    ir->disableRx();
  sendJson(200, "{\"ok\":true}");
}

void handleIrLearnPoll() {
  auto *ir = irHw();
  if (!ir) {
    sendJson(500, "{\"error\":\"no ir\"}");
    return;
  }
  const auto &cap = ir->lastCapture();
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  if (cap.valid) {
    d.AddMember("ok", true, a);
    d.AddMember("protocol", rapidjson::Value(cap.protocol.c_str(), a), a);
    d.AddMember("code", rapidjson::Value(cap.dataHex.c_str(), a), a);
    d.AddMember("human", rapidjson::Value(cap.human.c_str(), a), a);
  } else {
    d.AddMember("ok", false, a);
  }
  sendJson(200, OMOTE::JSON::ToString(d));
}

void handleRoot() {
  sendJson(200, "{\"name\":\"OMOTE\",\"api\":\"/api/status\"}");
}

void registerRoutes() {
  static bool registered = false;
  if (registered)
    return;
  registered = true;
  server.on("/api/status", HTTP_GET, handleStatus);
  server.on("/api/fs/tree", HTTP_GET, handleFsTree);
  server.on("/api/fs/read", HTTP_GET, handleFsRead);
  server.on("/api/fs/stat", HTTP_GET, handleFsStat);
  server.on("/api/fs/read/raw", HTTP_GET, handleFsReadRaw);
  server.on("/api/fs/read/chunk", HTTP_GET, handleFsReadChunk);
  server.on("/api/fs/write", HTTP_POST, handleFsWrite);
  server.on("/api/fs/write", HTTP_PUT, handleFsWrite);
  server.on("/api/fs/delete", HTTP_POST, handleFsDelete);
  server.on("/api/device/reboot", HTTP_POST, handleReboot);
  server.on("/api/device/settings", HTTP_GET, handleDeviceSettingsGet);
  server.on("/api/device/settings", HTTP_POST, handleDeviceSettingsPost);
  server.on("/api/device/settings/schema", HTTP_GET, handleDeviceSettingsSchemaGet);
  server.on("/api/device/sync-mode", HTTP_GET, handleEditorSyncGet);
  server.on("/api/device/sync-mode", HTTP_POST, handleEditorSyncPost);
  server.on("/api/ir/learn/start", HTTP_POST, handleIrLearnStart);
  server.on("/api/ir/learn/stop", HTTP_POST, handleIrLearnStop);
  server.on("/api/ir/learn/poll", HTTP_GET, handleIrLearnPoll);
  server.on("/", HTTP_GET, handleRoot);
  server.onNotFound([]() { sendJson(404, "{\"error\":\"not found\"}"); });
  server.on("/api/status", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/tree", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/read", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/stat", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/read/raw", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/read/chunk", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/write", HTTP_OPTIONS, handleOptions);
  server.on("/api/fs/delete", HTTP_OPTIONS, handleOptions);
  server.on("/api/device/reboot", HTTP_OPTIONS, handleOptions);
  server.on("/api/device/settings", HTTP_OPTIONS, handleOptions);
  server.on("/api/device/settings/schema", HTTP_OPTIONS, handleOptions);
  server.on("/api/device/sync-mode", HTTP_OPTIONS, handleOptions);
  server.on("/api/ir/learn/start", HTTP_OPTIONS, handleOptions);
  server.on("/api/ir/learn/stop", HTTP_OPTIONS, handleOptions);
  server.on("/api/ir/learn/poll", HTTP_OPTIONS, handleOptions);
}

} // namespace

namespace config_http {

void begin(const char *mdnsName) {
  if (mdnsName && mdnsName[0])
    mdnsHost = mdnsName;
  if (!logger)
    logger = std::make_unique<LoggingInterface>();
}

void sync() {
  if (rebootAtMs && (int32_t)(millis() - rebootAtMs) >= 0) {
    ESP.restart();
  }

  if (!WiFi.isConnected()) {
    if (running) {
      server.stop();
      running = false;
    }
    if (mdnsStarted) {
      MDNS.end();
      mdnsStarted = false;
    }
    return;
  }

  if (!mdnsStarted) {
    if (MDNS.begin(mdnsHost.c_str())) {
      MDNS.addService("http", "tcp", 80);
      mdnsStarted = true;
      if (logger)
        logger->info("HTTP config API at http://" + mdnsHost + ".local/api/status");
    }
  }

  if (!running) {
    registerRoutes();
    server.begin();
    running = true;
  }
  const int passes = editor_sync_mode::isActive() ? 3 : 2;
  for (int i = 0; i < passes; i++)
    server.handleClient();

  if (isRemoteSessionActive()) {
    if (auto disp = std::static_pointer_cast<Display>(HardwareFactory::getAbstract().display())) {
      disp->getTouchData();
      disp->pokeTouchController();
    }
  }
}

void stop() {
  if (running) {
    server.stop();
    running = false;
  }
}

bool isRunning() { return running; }

bool isRemoteSessionActive() {
  if (!lastRemoteHttpMs)
    return false;
  return (millis() - lastRemoteHttpMs) < 120000;
}

} // namespace config_http
#endif // !IS_SIMULATOR

