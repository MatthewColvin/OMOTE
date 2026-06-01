#include "HaRuntime.hpp"



#include "HaWebSocket.hpp"

#include "JsonPage.hpp"

#include "RapidJsonUtilty.hpp"



#include <Arduino.h>

#include <algorithm>

#include <filesystem>

#include <fstream>



namespace {



struct Settings {

  std::string url;

  std::string token;

  bool ok() const { return !url.empty() && !token.empty(); }

};



Settings gSettings;

std::vector<std::string> gActiveEntities;

UI::Page::JsonPage *gActivePage = nullptr;



struct StateEntry {

  std::string entityId;

  std::string state;

  std::string attributesJson;

};



std::vector<StateEntry> gStates;

uint32_t gLastCacheSaveMs = 0;

bool gDirtyCache = false;

volatile bool gUiRefreshPending = false;



static constexpr uint32_t kCacheSaveMs = 60000;

static constexpr size_t kMaxStates = 24;



std::string trimSlashes(std::string s) {

  while (!s.empty() && s.back() == '/')

    s.pop_back();

  return s;

}



Settings loadSettingsFile() {

  Settings s;

  std::filesystem::path path(FS_PATH "HaSettings.json");

  rapidjson::Document d = OMOTE::JSON::GetDocument(path);

  if (d.HasParseError() || d.IsNull())

    return s;

  if (d.HasMember("Url") && d["Url"].IsString())

    s.url = trimSlashes(d["Url"].GetString());

  if (d.HasMember("Token") && d["Token"].IsString())

    s.token = d["Token"].GetString();

  return s;

}



void loadStateCacheFile() {

  std::filesystem::path path(FS_PATH "HaStateCache.json");

  rapidjson::Document d = OMOTE::JSON::GetDocument(path);

  if (d.HasParseError() || !d.IsObject())

    return;

  for (auto it = d.MemberBegin(); it != d.MemberEnd(); ++it) {

    if (!it->name.IsString() || !it->value.IsString())

      continue;

    gStates.push_back({it->name.GetString(), it->value.GetString()});

    if (gStates.size() >= kMaxStates)

      break;

  }

}



void saveStateCacheFile() {

  if (!gDirtyCache)

    return;

  rapidjson::Document d;

  d.SetObject();

  auto &a = d.GetAllocator();

  for (const auto &e : gStates)

    d.AddMember(rapidjson::Value(e.entityId.c_str(), a), rapidjson::Value(e.state.c_str(), a), a);

  std::filesystem::path path(FS_PATH "HaStateCache.json");

  std::ofstream out(path);

  if (!out)

    return;

  out << OMOTE::JSON::ToString(d);

  gDirtyCache = false;

  gLastCacheSaveMs = millis();

}



StateEntry *findState(const std::string &entityId) {

  for (auto &e : gStates) {

    if (e.entityId == entityId)

      return &e;

  }

  return nullptr;

}



void upsertState(const std::string &entityId, const std::string &state, const std::string &attributesJson) {

  if (entityId.empty())

    return;

  if (auto *e = findState(entityId)) {

    bool changed = e->state != state;

    if (!attributesJson.empty() && e->attributesJson != attributesJson) {

      e->attributesJson = attributesJson;

      changed = true;

    }

    if (e->state != state) {

      e->state = state;

      changed = true;

    }

    if (changed) {

      gDirtyCache = true;

      gUiRefreshPending = true;

    }

    return;

  }

  if (gStates.size() >= kMaxStates)

    gStates.erase(gStates.begin());

  gStates.push_back({entityId, state, attributesJson});

  gDirtyCache = true;

  gUiRefreshPending = true;

}



void onWsState(const std::string &entityId, const std::string &state, const std::string &attributesJson) {
  upsertState(entityId, state, attributesJson);
}



void refreshSettings() {
  gSettings = loadSettingsFile();
  HaWebSocket::setSettings(gSettings.url, gSettings.token);
}

void pushSubscription() {
  HaWebSocket::subscribeEntities(gActiveEntities);
}

} // namespace



namespace HaRuntime {



void init() {
  refreshSettings();
  loadStateCacheFile();
  gLastCacheSaveMs = millis();

  HaWebSocket::setStateCallback(onWsState);
  HaWebSocket::start();
  if (configured())
    Serial.println("HA> HaSettings loaded");
  else
    Serial.println("HA> HaSettings.json missing Url or Token");
}



void reloadSettingsFromDisk() {
  refreshSettings();
  if (gSettings.ok()) {
    Serial.println("HA> settings reloaded from LittleFS");
    if (!gActiveEntities.empty())
      pushSubscription();
  } else {
    Serial.println("HA> settings reload: HaSettings.json missing Url or Token");
  }
}

void tick() {
  HaWebSocket::tick();

  const uint32_t now = millis();
  static uint32_t sLastSettingsReload = 0;
  if (now - sLastSettingsReload >= 30000) {
    sLastSettingsReload = now;
    refreshSettings();
  }

  if (!configured())
    return;

  if (gUiRefreshPending && gActivePage) {
    gUiRefreshPending = false;
    gActivePage->applyHaStates();
  }

  if (gDirtyCache && now - gLastCacheSaveMs >= kCacheSaveMs)
    saveStateCacheFile();
}



bool configured() { return gSettings.ok(); }



bool callService(const std::string &domain, const std::string &service, const std::string &entityId) {
  refreshSettings();
  if (entityId.empty())
    return false;
  if (!configured()) {
    Serial.println("HA> tap ignored: save HA settings to remote (HaSettings.json on LittleFS)");
    return false;
  }

  std::string dom = domain;
  if (dom.empty()) {
    const auto dot = entityId.find('.');
    dom = dot != std::string::npos ? entityId.substr(0, dot) : "homeassistant";
  }
  std::string svc = service.empty() ? "toggle" : service;

  Serial.printf("HA> tap %s.%s %s\n", dom.c_str(), svc.c_str(), entityId.c_str());
  return HaWebSocket::callServiceRest(dom, svc, entityId);
}

bool callServiceWithData(const std::string &domain, const std::string &service, const std::string &entityId,
                         const std::string &serviceDataJson) {
  refreshSettings();
  if (entityId.empty())
    return false;
  if (!configured()) {
    Serial.println("HA> tap ignored: save HA settings to remote (HaSettings.json on LittleFS)");
    return false;
  }

  std::string dom = domain;
  if (dom.empty()) {
    const auto dot = entityId.find('.');
    dom = dot != std::string::npos ? entityId.substr(0, dot) : "homeassistant";
  }
  std::string svc = service.empty() ? "turn_on" : service;

  Serial.printf("HA> call %s.%s %s\n", dom.c_str(), svc.c_str(), entityId.c_str());
  return HaWebSocket::callServiceRestWithData(dom, svc, entityId, serviceDataJson);
}

bool getCachedAttributes(const std::string &entityId, std::string &attributesJsonOut) {
  if (auto *e = findState(entityId)) {
    if (!e->attributesJson.empty()) {
      attributesJsonOut = e->attributesJson;
      return true;
    }
  }
  return false;
}

bool fetchEntityState(const std::string &entityId) {
  std::string state;
  std::string attrs;
  if (!HaWebSocket::fetchEntityStateRest(entityId, state, attrs))
    return false;
  upsertState(entityId, state, attrs);
  return true;
}



bool getCachedState(const std::string &entityId, std::string &stateOut) {

  if (auto *e = findState(entityId)) {

    stateOut = e->state;

    return true;

  }

  return false;

}



bool stateIsOn(const std::string &entityId, const std::string &state) {

  std::string s = state;

  std::transform(s.begin(), s.end(), s.begin(), ::tolower);

  if (s == "unavailable" || s == "unknown")

    return false;



  const auto dot = entityId.find('.');

  const std::string dom = dot != std::string::npos ? entityId.substr(0, dot) : "";

  if (dom == "cover")

    return s == "open" || s == "opening";

  if (dom == "lock")

    return s == "unlocked";

  if (dom == "climate")

    return s != "off";

  return s == "on" || s == "true" || s == "1" || s == "playing" || s == "home" || s == "heat" || s == "cool";

}



void setActivePage(UI::Page::JsonPage *page, const std::vector<std::string> &entityIds) {
  gActivePage = page;
  gActiveEntities = entityIds;
  refreshSettings();
  if (configured())
    pushSubscription();
}



void requestRefresh() {

  if (gActivePage)

    gActivePage->applyHaStates();

  if (configured())

    pushSubscription();

}



} // namespace HaRuntime

