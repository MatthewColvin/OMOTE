#include "config_http.hpp"

#include "Hardware/LoggingInterface.hpp"
#include "HardwareFactory.hpp"
#include "RapidJsonUtilty.hpp"
#include "editor_sync_mode.hpp"
#include "ir/IRTransceiver.hpp"

#include <Arduino.h>
#include <ESPmDNS.h>
#include <LittleFS.h>
#include <WebServer.h>
#include <WiFi.h>

#include <algorithm>
#include <fstream>
#include <sstream>
#include <vector>

namespace {

#ifndef FS_PATH
#define FS_PATH "/littlefs/"
#endif

WebServer server(80);
bool running = false;
bool mdnsStarted = false;
std::string mdnsHost = "omote";
uint32_t rebootAtMs = 0;

std::unique_ptr<LoggingInterface> logger;

IRTransceiver *irHw() {
  return static_cast<IRTransceiver *>(HardwareFactory::getAbstract().ir().get());
}

void sendCors() {
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.sendHeader("Access-Control-Allow-Methods", "GET, POST, PUT, OPTIONS");
  server.sendHeader("Access-Control-Allow-Headers", "Content-Type");
}

void sendJson(int code, const std::string &body) {
  sendCors();
  server.sendHeader("Connection", "close");
  server.send(code, "application/json", body.c_str());
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
  }
  root.close();
}

void handleOptions() {
  sendCors();
  server.send(204);
}

void handleStatus() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("connected", WiFi.isConnected(), a);
  d.AddMember("ip", rapidjson::Value(WiFi.localIP().toString().c_str(), a), a);
  d.AddMember("hostname", rapidjson::Value(mdnsHost.c_str(), a), a);
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
      arr.PushBack(rapidjson::Value(f.c_str(), a), a);
  }
  std::sort(files.begin(), files.end());
  arr.Clear();
  for (const auto &f : files) {
    if (f.rfind("editor/", 0) == 0)
      continue;
    if (f.size() >= 5 && f.substr(f.size() - 5) == ".json")
      arr.PushBack(rapidjson::Value(f.c_str(), a), a);
  }
  d.AddMember("files", arr, a);
  sendJson(200, OMOTE::JSON::ToString(d));
}

void handleFsRead() {
  if (!server.hasArg("path")) {
    sendJson(400, "{\"error\":\"missing path\"}");
    return;
  }
  std::string path = server.arg("path").c_str();
  if (!isSafePath(path)) {
    sendJson(400, "{\"error\":\"invalid path\"}");
    return;
  }

  std::ifstream file(vfsPath(path), std::ios::in);
  if (!file) {
    sendJson(404, "{\"error\":\"not found\"}");
    return;
  }
  std::stringstream buffer;
  buffer << file.rdbuf();
  file.close();

  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("path", rapidjson::Value(path.c_str(), a), a);
  d.AddMember("content", rapidjson::Value(buffer.str().c_str(), a), a);
  sendJson(200, OMOTE::JSON::ToString(d));
}

void handleFsWrite() {
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
  sendJson(200, "{\"ok\":true}");
}

void handleReboot() {
  rebootAtMs = millis() + 500;
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
  if (on) {
    editor_sync_mode::enter();
    sendJson(200, "{\"ok\":true,\"editor_sync\":true}");
  } else {
    sendJson(200, "{\"ok\":true,\"editor_sync\":false,\"restart\":true}");
    rebootAtMs = millis() + 500;
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
  server.on("/api/fs/write", HTTP_POST, handleFsWrite);
  server.on("/api/fs/write", HTTP_PUT, handleFsWrite);
  server.on("/api/device/reboot", HTTP_POST, handleReboot);
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
  server.on("/api/fs/write", HTTP_OPTIONS, handleOptions);
  server.on("/api/device/reboot", HTTP_OPTIONS, handleOptions);
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
  const int passes = editor_sync_mode::isActive() ? 16 : 2;
  for (int i = 0; i < passes; i++)
    server.handleClient();
}

void stop() {
  if (running) {
    server.stop();
    running = false;
  }
}

bool isRunning() { return running; }

} // namespace config_http
