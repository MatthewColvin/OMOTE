#if !defined(IS_SIMULATOR)

#include "HaWebSocket.hpp"

#include "editor_sync_mode.hpp"
#include "RapidJsonUtilty.hpp"

#include <Arduino.h>
#include <HTTPClient.h>
#include <WiFi.h>
#include <WiFiUdp.h>
#include <cstring>
#include <freertos/FreeRTOS.h>
#include <freertos/queue.h>
#include <freertos/semphr.h>

#include "esp_websocket_client.h"

namespace {

struct Settings {
  std::string url;
  std::string token;
  bool ok() const { return !url.empty() && !token.empty(); }
};

Settings gSettings;
esp_websocket_client_handle_t gClient = nullptr;
std::string gWsUri;
std::string gIncoming;
bool gAuthOk = false;
bool gConnecting = false;
bool gWantConnect = false;
uint32_t gLastConnectAttemptMs = 0;
uint32_t gNetworkReadyMs = 0;
uint32_t gNextMsgId = 1;
std::vector<std::string> gSubscribed;
QueueHandle_t gCmdQueue = nullptr;
SemaphoreHandle_t gSubMx = nullptr;
HaWebSocket::StateCallback gStateCallback;

static constexpr uint32_t kConnectCooldownMs = 5000;
static constexpr uint32_t kNetworkSettleMs = 4000;
static constexpr uint32_t kMinHeapBytes = 28000;

enum CmdType : uint8_t {
  CMD_SUBSCRIBE = 1,
  CMD_CALL_SERVICE = 2,
};

struct WsCommand {
  CmdType type = CMD_SUBSCRIBE;
  char domain[20] = {};
  char service[24] = {};
  char entityId[96] = {};
};

std::string trimSlashes(std::string s) {
  while (!s.empty() && s.back() == '/')
    s.pop_back();
  return s;
}

bool heapOk() { return ESP.getFreeHeap() >= kMinHeapBytes; }

bool wifiReady() { return WiFi.status() == WL_CONNECTED; }

bool networkSettled() {
  return gNetworkReadyMs != 0 && (millis() - gNetworkReadyMs) >= kNetworkSettleMs;
}

uint32_t nextMsgId() { return gNextMsgId++; }

void notifyState(const std::string &entityId, const std::string &state, const std::string &attributesJson) {
  if (gStateCallback)
    gStateCallback(entityId, state, attributesJson);
}

void copySubscribeList(const std::vector<std::string> &entities) {
  if (!gSubMx)
    gSubMx = xSemaphoreCreateMutex();
  if (xSemaphoreTake(gSubMx, pdMS_TO_TICKS(100)) != pdTRUE)
    return;
  gSubscribed = entities;
  xSemaphoreGive(gSubMx);
}

bool entityIsSubscribed(const char *entityId) {
  if (!entityId || !entityId[0])
    return false;
  if (xSemaphoreTake(gSubMx, pdMS_TO_TICKS(20)) != pdTRUE)
    return false;
  bool found = false;
  for (const auto &e : gSubscribed) {
    if (e == entityId) {
      found = true;
      break;
    }
  }
  xSemaphoreGive(gSubMx);
  return found;
}

bool parseHaBaseUrl(const std::string &urlIn, std::string &host, uint16_t &port, bool &useSsl) {
  std::string url = urlIn;
  useSsl = url.rfind("https://", 0) == 0;
  if (url.rfind("http://", 0) == 0)
    url.erase(0, 7);
  else if (useSsl)
    url.erase(0, 8);
  url = trimSlashes(url);

  const auto colon = url.find(':');
  if (colon != std::string::npos && colon > 0) {
    host = url.substr(0, colon);
    port = static_cast<uint16_t>(atoi(url.substr(colon + 1).c_str()));
    if (port == 0)
      port = useSsl ? 443 : 8123;
  } else {
    host = url;
    port = useSsl ? 443 : 8123;
  }
  return !host.empty();
}

bool resolveHostToIp(const std::string &host, std::string &ipOut) {
  if (host.find('.') != std::string::npos && host != "homeassistant.local") {
    IPAddress addr;
    if (addr.fromString(host.c_str())) {
      ipOut = host;
      return true;
    }
  }
  IPAddress ip;
  if (WiFi.hostByName(host.c_str(), ip) != 1)
    return false;
  ipOut = ip.toString().c_str();
  return !ipOut.empty();
}

bool wsSendText(const std::string &msg) {
  if (!gClient || !esp_websocket_client_is_connected(gClient))
    return false;
  return esp_websocket_client_send_text(gClient, msg.c_str(), msg.length(), pdMS_TO_TICKS(2000)) >= 0;
}

void wsSendAuth() {
  if (gSettings.token.empty())
    return;
  rapidjson::Document doc;
  doc.SetObject();
  auto &a = doc.GetAllocator();
  doc.AddMember("type", rapidjson::Value("auth", a), a);
  doc.AddMember("access_token", rapidjson::Value(gSettings.token.c_str(), a), a);
  wsSendText(OMOTE::JSON::ToString(doc));
}

void wsDisconnect();

void wsSendSubscribeEntities() {
  if (!gAuthOk)
    return;
  if (xSemaphoreTake(gSubMx, pdMS_TO_TICKS(50)) != pdTRUE)
    return;
  if (gSubscribed.empty()) {
    xSemaphoreGive(gSubMx);
    return;
  }

  rapidjson::Document doc;
  doc.SetObject();
  auto &a = doc.GetAllocator();
  doc.AddMember("id", rapidjson::Value().SetUint(nextMsgId()), a);
  doc.AddMember("type", rapidjson::Value("subscribe_entities", a), a);
  rapidjson::Value arr(rapidjson::kArrayType);
  for (const auto &e : gSubscribed)
    arr.PushBack(rapidjson::Value(e.c_str(), a), a);
  doc.AddMember("entity_ids", arr, a);
  const size_t nEnt = gSubscribed.size();
  xSemaphoreGive(gSubMx);

  wsSendText(OMOTE::JSON::ToString(doc));
  Serial.printf("ha_ws: subscribe_entities (%u)\n", static_cast<unsigned>(nEnt));
}

void handleCompressedEntity(const char *entityId, const rapidjson::Value &ent) {
  if (!entityId || !entityId[0] || !entityIsSubscribed(entityId))
    return;
  const char *st = nullptr;
  if (ent.IsObject()) {
    if (ent.HasMember("s") && ent["s"].IsString())
      st = ent["s"].GetString();
    else if (ent.HasMember("state") && ent["state"].IsString())
      st = ent["state"].GetString();
  }
  if (st && st[0])
    notifyState(entityId, st, {});
}

void handleStateChanged(const rapidjson::Value &eventData) {
  if (!eventData.IsObject() || !eventData.HasMember("entity_id") || !eventData["entity_id"].IsString())
    return;
  const char *eid = eventData["entity_id"].GetString();
  if (!entityIsSubscribed(eid))
    return;
  if (!eventData.HasMember("new_state") || !eventData["new_state"].IsObject())
    return;
  const auto &newState = eventData["new_state"];
  if (!newState.HasMember("state") || !newState["state"].IsString())
    return;
  std::string attrsJson;
  if (newState.HasMember("attributes") && newState["attributes"].IsObject()) {
    rapidjson::Document attrsDoc;
    attrsDoc.CopyFrom(newState["attributes"], attrsDoc.GetAllocator());
    attrsJson = OMOTE::JSON::ToString(attrsDoc);
  }
  notifyState(eid, newState["state"].GetString(), attrsJson);
}

void handleWsText(const char *payload) {
  rapidjson::Document doc;
  if (doc.Parse(payload).HasParseError() || !doc.IsObject())
    return;

  const char *type = doc.HasMember("type") && doc["type"].IsString() ? doc["type"].GetString() : "";
  if (strcmp(type, "auth_required") == 0) {
    wsSendAuth();
    return;
  }
  if (strcmp(type, "auth_ok") == 0) {
    gAuthOk = true;
    gConnecting = false;
    Serial.println("ha_ws: authenticated");
    wsSendSubscribeEntities();
    return;
  }
  if (strcmp(type, "auth_invalid") == 0) {
    Serial.println("ha_ws: auth invalid");
    gAuthOk = false;
    wsDisconnect();
    return;
  }
  if (strcmp(type, "event") != 0 || !doc.HasMember("event") || !doc["event"].IsObject())
    return;

  const auto &event = doc["event"];
  if (event.HasMember("event_type") && event["event_type"].IsString() &&
      strcmp(event["event_type"].GetString(), "state_changed") == 0) {
    if (event.HasMember("data") && event["data"].IsObject())
      handleStateChanged(event["data"]);
    return;
  }

  if (event.HasMember("a") && event["a"].IsObject()) {
    for (auto it = event["a"].MemberBegin(); it != event["a"].MemberEnd(); ++it)
      handleCompressedEntity(it->name.GetString(), it->value);
  }
}

void processWsData(esp_websocket_event_data_t *data) {
  if (!data || data->data_len <= 0)
    return;

  if (data->payload_offset == 0)
    gIncoming.clear();

  gIncoming.append(data->data_ptr, static_cast<size_t>(data->data_len));

  if (gIncoming.size() >= static_cast<size_t>(data->payload_len)) {
    handleWsText(gIncoming.c_str());
    gIncoming.clear();
  }
}

void wsEventHandler(void *, esp_event_base_t, int32_t eventId, void *eventData) {
  switch (eventId) {
  case WEBSOCKET_EVENT_CONNECTED:
    gConnecting = false;
    Serial.println("ha_ws: connected");
    break;
  case WEBSOCKET_EVENT_DISCONNECTED:
    gAuthOk = false;
    gConnecting = false;
    Serial.println("ha_ws: disconnected");
    break;
  case WEBSOCKET_EVENT_DATA:
    processWsData(static_cast<esp_websocket_event_data_t *>(eventData));
    break;
  default:
    break;
  }
}

void wsDisconnect() {
  if (gClient) {
    esp_websocket_client_stop(gClient);
    esp_websocket_client_destroy(gClient);
    gClient = nullptr;
  }
  gAuthOk = false;
  gConnecting = false;
  gIncoming.clear();
}

bool wsConnect() {
  if (!gSettings.ok() || !wifiReady())
    return false;
  if (!networkSettled() || !heapOk())
    return false;

  const uint32_t now = millis();
  if (now - gLastConnectAttemptMs < kConnectCooldownMs)
    return false;
  gLastConnectAttemptMs = now;

  std::string host;
  uint16_t port = 8123;
  bool ssl = false;
  if (!parseHaBaseUrl(gSettings.url, host, port, ssl))
    return false;

  std::string ip;
  if (!resolveHostToIp(host, ip))
    return false;

  wsDisconnect();

  gWsUri = (ssl ? "wss://" : "ws://") + ip + ":" + std::to_string(port) + "/api/websocket";

  esp_websocket_client_config_t cfg = {};
  cfg.uri = gWsUri.c_str();
  cfg.reconnect_timeout_ms = 0;
  cfg.network_timeout_ms = 8000;

  gClient = esp_websocket_client_init(&cfg);
  if (!gClient)
    return false;

  esp_websocket_register_events(gClient, WEBSOCKET_EVENT_ANY, wsEventHandler, nullptr);
  gAuthOk = false;
  gConnecting = true;

  if (esp_websocket_client_start(gClient) != ESP_OK) {
    wsDisconnect();
    return false;
  }

  Serial.printf("ha_ws: connecting %s (%s) heap=%u\n", host.c_str(), ip.c_str(),
                static_cast<unsigned>(ESP.getFreeHeap()));
  return true;
}

bool restCallServiceWithData(const std::string &domain, const std::string &service, const std::string &entityId,
                             const std::string &serviceDataJson) {
  if (!gSettings.ok()) {
    Serial.println("HA> REST skip: HaSettings.json missing Url or Token");
    return false;
  }
  if (!wifiReady()) {
    Serial.println("HA> REST skip: WiFi not connected");
    return false;
  }

  rapidjson::Document body;
  body.SetObject();
  auto &a = body.GetAllocator();
  body.AddMember("entity_id", rapidjson::Value(entityId.c_str(), a), a);

  if (!serviceDataJson.empty()) {
    rapidjson::Document data;
    data.Parse(serviceDataJson.c_str());
    if (!data.HasParseError() && data.IsObject()) {
      for (auto it = data.MemberBegin(); it != data.MemberEnd(); ++it) {
        rapidjson::Value name(it->name, a);
        rapidjson::Value val(it->value, a);
        body.AddMember(name, val, a);
      }
    }
  }

  HTTPClient http;
  const std::string url = gSettings.url + "/api/services/" + domain + "/" + service;
  if (!http.begin(url.c_str())) {
    Serial.printf("HA> REST begin failed: %s\n", url.c_str());
    return false;
  }
  http.addHeader("Authorization", ("Bearer " + gSettings.token).c_str());
  http.addHeader("Content-Type", "application/json");
  http.setTimeout(8000);
  const int code = http.POST(OMOTE::JSON::ToString(body).c_str());
  http.end();
  Serial.printf("HA> REST %s.%s %s -> HTTP %d\n", domain.c_str(), service.c_str(), entityId.c_str(), code);
  return code >= 200 && code < 300;
}

bool restCallService(const std::string &domain, const std::string &service, const std::string &entityId) {
  return restCallServiceWithData(domain, service, entityId, {});
}

bool fetchEntityStateRest(const std::string &entityId, std::string &stateOut, std::string &attributesJsonOut) {
  stateOut.clear();
  attributesJsonOut.clear();
  if (!gSettings.ok() || !wifiReady() || entityId.empty())
    return false;

  HTTPClient http;
  const std::string url = gSettings.url + "/api/states/" + entityId;
  if (!http.begin(url.c_str()))
    return false;
  http.addHeader("Authorization", ("Bearer " + gSettings.token).c_str());
  http.setTimeout(8000);
  const int code = http.GET();
  const String payload = http.getString();
  http.end();
  if (code < 200 || code >= 300)
    return false;

  rapidjson::Document doc;
  if (doc.Parse(payload.c_str()).HasParseError() || !doc.IsObject())
    return false;
  if (!doc.HasMember("state") || !doc["state"].IsString())
    return false;
  stateOut = doc["state"].GetString();
  if (doc.HasMember("attributes") && doc["attributes"].IsObject()) {
    rapidjson::Document attrsDoc;
    attrsDoc.CopyFrom(doc["attributes"], attrsDoc.GetAllocator());
    attributesJsonOut = OMOTE::JSON::ToString(attrsDoc);
  }
  return true;
}

void wsSendCallService(const WsCommand &cmd) {
  rapidjson::Document doc;
  doc.SetObject();
  auto &a = doc.GetAllocator();
  doc.AddMember("id", rapidjson::Value().SetUint(nextMsgId()), a);
  doc.AddMember("type", rapidjson::Value("call_service", a), a);
  doc.AddMember("domain", rapidjson::Value(cmd.domain, a), a);
  doc.AddMember("service", rapidjson::Value(cmd.service, a), a);
  rapidjson::Value target(rapidjson::kObjectType);
  target.AddMember("entity_id", rapidjson::Value(cmd.entityId, a), a);
  doc.AddMember("target", target, a);
  wsSendText(OMOTE::JSON::ToString(doc));
}

void processCommand(const WsCommand &cmd) {
  switch (cmd.type) {
  case CMD_SUBSCRIBE:
    if (xSemaphoreTake(gSubMx, pdMS_TO_TICKS(50)) == pdTRUE) {
      const bool empty = gSubscribed.empty();
      xSemaphoreGive(gSubMx);
      if (empty) {
        gWantConnect = false;
        wsDisconnect();
        break;
      }
    }
    gWantConnect = true;
    if (gAuthOk)
      wsSendSubscribeEntities();
    else if (!gConnecting)
      wsConnect();
    break;
  case CMD_CALL_SERVICE:
    if (gAuthOk)
      wsSendCallService(cmd);
    else
      restCallService(cmd.domain, cmd.service, cmd.entityId);
    break;
  default:
    break;
  }
}

void queueCommand(const WsCommand &cmd) {
  if (gCmdQueue)
    xQueueSend(gCmdQueue, &cmd, 0);
}

} // namespace

namespace HaWebSocket {

void start() {
  if (!gCmdQueue)
    gCmdQueue = xQueueCreate(4, sizeof(WsCommand));
  if (!gSubMx)
    gSubMx = xSemaphoreCreateMutex();
  Serial.println("ha_ws: ready");
}

void tick() {
  if (wifiReady() && gSettings.ok() && !editor_sync_mode::isActive()) {
    if (gNetworkReadyMs == 0)
      gNetworkReadyMs = millis();
    if (gWantConnect && !gAuthOk && !gConnecting && networkSettled() &&
        millis() - gLastConnectAttemptMs >= kConnectCooldownMs) {
      wsConnect();
    }
  } else {
    if (editor_sync_mode::isActive() && gClient)
      wsDisconnect();
    if (gClient) {
      wsDisconnect();
      gWantConnect = false;
    }
    gNetworkReadyMs = 0;
  }

  WsCommand cmd;
  while (gCmdQueue && xQueueReceive(gCmdQueue, &cmd, 0) == pdTRUE)
    processCommand(cmd);
}

void setSettings(const std::string &url, const std::string &token) {
  gSettings.url = trimSlashes(url);
  gSettings.token = token;
}

void setStateCallback(StateCallback cb) { gStateCallback = std::move(cb); }

void subscribeEntities(const std::vector<std::string> &entityIds) {
  if (!gSettings.ok())
    return;
  copySubscribeList(entityIds);
  gWantConnect = !entityIds.empty();
  WsCommand cmd;
  cmd.type = CMD_SUBSCRIBE;
  queueCommand(cmd);
}

bool isConnected() { return gAuthOk; }

bool callService(const std::string &domain, const std::string &service, const std::string &entityId) {
  if (!gSettings.ok() || entityId.empty())
    return false;

  WsCommand cmd;
  cmd.type = CMD_CALL_SERVICE;
  strncpy(cmd.domain, domain.c_str(), sizeof(cmd.domain) - 1);
  strncpy(cmd.service, service.c_str(), sizeof(cmd.service) - 1);
  strncpy(cmd.entityId, entityId.c_str(), sizeof(cmd.entityId) - 1);
  if (gCmdQueue && xQueueSend(gCmdQueue, &cmd, 0) == pdTRUE)
    return true;
  Serial.println("HA> WS queue full, use REST");
  return false;
}

bool callServiceRest(const std::string &domain, const std::string &service, const std::string &entityId) {
  return restCallService(domain, service, entityId);
}

bool callServiceRestWithData(const std::string &domain, const std::string &service, const std::string &entityId,
                             const std::string &serviceDataJson) {
  return restCallServiceWithData(domain, service, entityId, serviceDataJson);
}

bool fetchEntityStateRest(const std::string &entityId, std::string &stateOut, std::string &attributesJsonOut) {
  return ::fetchEntityStateRest(entityId, stateOut, attributesJsonOut);
}

} // namespace HaWebSocket

#endif // !IS_SIMULATOR
