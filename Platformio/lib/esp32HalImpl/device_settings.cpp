#include "device_settings.hpp"

#include "HardwareFactory.hpp"
#include "RapidJsonUtilty.hpp"
#include "display.hpp"
#include "omoteconfig.h"

#include <fstream>

extern "C" unsigned long millis(void);

#ifndef FS_PATH
#define FS_PATH "/littlefs/"
#endif

namespace device_settings {

namespace {

Settings sSettings;
uint32_t sLastActivityMs = 0;
bool sScreenPoweredOff = false;

std::string vfsPath(const char *rel) {
  std::string base = FS_PATH;
  if (!base.empty() && base.back() == '/')
    base.pop_back();
  return base + "/" + rel;
}

void clampDeepSleep() {
  const uint32_t minDeep = sSettings.displayTimeoutMs + 60000;
  if (sSettings.deepSleepTimeoutMs < minDeep)
    sSettings.deepSleepTimeoutMs = minDeep;
}

bool tryGetUint32(const rapidjson::Value &doc, const char *key, uint32_t &out) {
  if (!doc.HasMember(key))
    return false;
  const auto &v = doc[key];
  if (v.IsUint()) {
    out = v.GetUint();
    return true;
  }
  if (v.IsInt() && v.GetInt() >= 0) {
    out = (uint32_t)v.GetInt();
    return true;
  }
  if (v.IsNumber()) {
    out = (uint32_t)v.GetDouble();
    return true;
  }
  return false;
}

bool tryGetUint8(const rapidjson::Value &doc, const char *key, uint8_t &out) {
  uint32_t tmp = 0;
  if (!tryGetUint32(doc, key, tmp))
    return false;
  out = (uint8_t)tmp;
  return true;
}

bool tryGetInt32(const rapidjson::Value &doc, const char *key, int32_t &out) {
  if (!doc.HasMember(key))
    return false;
  const auto &v = doc[key];
  if (v.IsInt()) {
    out = v.GetInt();
    return true;
  }
  if (v.IsUint()) {
    out = static_cast<int32_t>(v.GetUint());
    return true;
  }
  if (v.IsNumber()) {
    out = static_cast<int32_t>(v.GetDouble());
    return true;
  }
  return false;
}

bool tryGetString(const rapidjson::Value &doc, const char *key, std::string &out) {
  if (!doc.HasMember(key) || !doc[key].IsString())
    return false;
  out = doc[key].GetString();
  return true;
}

} // namespace

Settings &current() { return sSettings; }
const Settings &currentConst() { return sSettings; }

bool mergeFromJson(const rapidjson::Value &doc) {
  if (!doc.IsObject())
    return false;

  tryGetUint32(doc, "display_timeout_ms", sSettings.displayTimeoutMs);
  tryGetUint32(doc, "deep_sleep_timeout_ms", sSettings.deepSleepTimeoutMs);
  tryGetUint32(doc, "dim_lead_ms", sSettings.dimLeadMs);
  tryGetUint32(doc, "light_sleep_timeout_ms", sSettings.lightSleepTimeoutMs);
  if (doc.HasMember("motion_wake_enabled") && doc["motion_wake_enabled"].IsBool())
    sSettings.motionWakeEnabled = doc["motion_wake_enabled"].GetBool();
  if (doc.HasMember("key_wake_enabled") && doc["key_wake_enabled"].IsBool())
    sSettings.keyWakeEnabled = doc["key_wake_enabled"].GetBool();
  if (doc.HasMember("light_sleep_enabled") && doc["light_sleep_enabled"].IsBool())
    sSettings.lightSleepEnabled = doc["light_sleep_enabled"].GetBool();
  tryGetUint8(doc, "lcd_day_brightness", sSettings.lcdDayBrightness);
  tryGetUint8(doc, "lcd_night_brightness", sSettings.lcdNightBrightness);
  tryGetUint8(doc, "kbd_day_brightness", sSettings.kbdDayBrightness);
  tryGetUint8(doc, "kbd_night_brightness", sSettings.kbdNightBrightness);
  if (doc.HasMember("mqtt_enabled") && doc["mqtt_enabled"].IsBool())
    sSettings.mqttEnabled = doc["mqtt_enabled"].GetBool();
  tryGetString(doc, "mqtt_broker", sSettings.mqttBroker);
  tryGetString(doc, "mqtt_port", sSettings.mqttPort);
  tryGetString(doc, "mqtt_user", sSettings.mqttUser);
  tryGetString(doc, "mqtt_password", sSettings.mqttPassword);
  tryGetString(doc, "mqtt_client_id", sSettings.mqttClientId);
  if (doc.HasMember("ntp_enabled") && doc["ntp_enabled"].IsBool())
    sSettings.ntpEnabled = doc["ntp_enabled"].GetBool();
  tryGetInt32(doc, "ntp_display_mode", sSettings.ntpDisplayMode);
  tryGetString(doc, "ntp_server", sSettings.ntpServer);
  tryGetString(doc, "timezone", sSettings.timezone);
  if (doc.HasMember("ftp_enabled") && doc["ftp_enabled"].IsBool())
    sSettings.ftpEnabled = doc["ftp_enabled"].GetBool();
  tryGetString(doc, "ftp_mdns_name", sSettings.ftpMdnsName);
  tryGetString(doc, "ftp_user", sSettings.ftpUser);
  tryGetString(doc, "ftp_password", sSettings.ftpPassword);

  clampDeepSleep();
  return true;
}

bool loadFromLittleFS() {
  const auto doc = OMOTE::JSON::GetDocument(std::filesystem::path(FS_PATH "DeviceSettings.json"));
  if (doc.HasParseError() || !doc.IsObject())
    return false;
  return mergeFromJson(doc);
}

rapidjson::Document toJsonDocument() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("display_timeout_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.displayTimeoutMs)), a);
  d.AddMember("deep_sleep_timeout_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.deepSleepTimeoutMs)), a);
  d.AddMember("dim_lead_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.dimLeadMs)), a);
  d.AddMember("motion_wake_enabled", sSettings.motionWakeEnabled, a);
  d.AddMember("key_wake_enabled", sSettings.keyWakeEnabled, a);
  d.AddMember("light_sleep_enabled", sSettings.lightSleepEnabled, a);
  d.AddMember("light_sleep_timeout_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.lightSleepTimeoutMs)), a);
  d.AddMember("lcd_day_brightness", static_cast<unsigned>(sSettings.lcdDayBrightness), a);
  d.AddMember("lcd_night_brightness", static_cast<unsigned>(sSettings.lcdNightBrightness), a);
  d.AddMember("kbd_day_brightness", static_cast<unsigned>(sSettings.kbdDayBrightness), a);
  d.AddMember("kbd_night_brightness", static_cast<unsigned>(sSettings.kbdNightBrightness), a);
  d.AddMember("mqtt_enabled", sSettings.mqttEnabled, a);
  d.AddMember("mqtt_broker", rapidjson::Value(sSettings.mqttBroker.c_str(), a), a);
  d.AddMember("mqtt_port", rapidjson::Value(sSettings.mqttPort.c_str(), a), a);
  d.AddMember("mqtt_user", rapidjson::Value(sSettings.mqttUser.c_str(), a), a);
  d.AddMember("mqtt_password", rapidjson::Value(sSettings.mqttPassword.c_str(), a), a);
  d.AddMember("mqtt_client_id", rapidjson::Value(sSettings.mqttClientId.c_str(), a), a);
  d.AddMember("ntp_enabled", sSettings.ntpEnabled, a);
  d.AddMember("ntp_display_mode", static_cast<int>(sSettings.ntpDisplayMode), a);
  d.AddMember("ntp_server", rapidjson::Value(sSettings.ntpServer.c_str(), a), a);
  d.AddMember("timezone", rapidjson::Value(sSettings.timezone.c_str(), a), a);
  d.AddMember("ftp_enabled", sSettings.ftpEnabled, a);
  d.AddMember("ftp_mdns_name", rapidjson::Value(sSettings.ftpMdnsName.c_str(), a), a);
  d.AddMember("ftp_user", rapidjson::Value(sSettings.ftpUser.c_str(), a), a);
  d.AddMember("ftp_password", rapidjson::Value(sSettings.ftpPassword.c_str(), a), a);
  return d;
}

bool saveToLittleFS() {
  rapidjson::Document d;
  d.SetObject();
  auto &a = d.GetAllocator();
  d.AddMember("display_timeout_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.displayTimeoutMs)), a);
  d.AddMember("deep_sleep_timeout_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.deepSleepTimeoutMs)), a);
  d.AddMember("dim_lead_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.dimLeadMs)), a);
  d.AddMember("motion_wake_enabled", sSettings.motionWakeEnabled, a);
  d.AddMember("key_wake_enabled", sSettings.keyWakeEnabled, a);
  d.AddMember("light_sleep_enabled", sSettings.lightSleepEnabled, a);
  d.AddMember("light_sleep_timeout_ms", rapidjson::Value(static_cast<uint64_t>(sSettings.lightSleepTimeoutMs)), a);
  d.AddMember("lcd_day_brightness", static_cast<unsigned>(sSettings.lcdDayBrightness), a);
  d.AddMember("lcd_night_brightness", static_cast<unsigned>(sSettings.lcdNightBrightness), a);
  d.AddMember("kbd_day_brightness", static_cast<unsigned>(sSettings.kbdDayBrightness), a);
  d.AddMember("kbd_night_brightness", static_cast<unsigned>(sSettings.kbdNightBrightness), a);
  d.AddMember("mqtt_enabled", sSettings.mqttEnabled, a);
  d.AddMember("mqtt_broker", rapidjson::Value(sSettings.mqttBroker.c_str(), a), a);
  d.AddMember("mqtt_port", rapidjson::Value(sSettings.mqttPort.c_str(), a), a);
  d.AddMember("mqtt_user", rapidjson::Value(sSettings.mqttUser.c_str(), a), a);
  d.AddMember("mqtt_password", rapidjson::Value(sSettings.mqttPassword.c_str(), a), a);
  d.AddMember("mqtt_client_id", rapidjson::Value(sSettings.mqttClientId.c_str(), a), a);
  d.AddMember("ntp_enabled", sSettings.ntpEnabled, a);
  d.AddMember("ntp_display_mode", static_cast<int>(sSettings.ntpDisplayMode), a);
  d.AddMember("ntp_server", rapidjson::Value(sSettings.ntpServer.c_str(), a), a);
  d.AddMember("timezone", rapidjson::Value(sSettings.timezone.c_str(), a), a);
  d.AddMember("ftp_enabled", sSettings.ftpEnabled, a);
  d.AddMember("ftp_mdns_name", rapidjson::Value(sSettings.ftpMdnsName.c_str(), a), a);
  d.AddMember("ftp_user", rapidjson::Value(sSettings.ftpUser.c_str(), a), a);
  d.AddMember("ftp_password", rapidjson::Value(sSettings.ftpPassword.c_str(), a), a);

  std::ofstream out(vfsPath("DeviceSettings.json"), std::ios::out | std::ios::trunc);
  if (!out)
    return false;
  const std::string body = OMOTE::JSON::ToString(d);
  out << body;
  return true;
}

void applyToHardware() {
  auto &hw = HardwareFactory::getAbstract();
  auto wifi = hw.wifi();
  hw.setWakeupByIMUEnabled(sSettings.motionWakeEnabled);
  hw.setLightSleepEnabled(sSettings.lightSleepEnabled);
  hw.setLightSleepTimeout(sSettings.lightSleepTimeoutMs);
  hw.setSleepTimeout(sSettings.displayTimeoutMs);

  auto disp = std::static_pointer_cast<Display>(hw.display());
  if (!disp)
    return;
  if (sSettings.lcdDayBrightness >= 10)
    disp->setLcdDayBrightness(sSettings.lcdDayBrightness, true);
  if (sSettings.lcdNightBrightness >= 10)
    disp->setLcdNightBrightness(sSettings.lcdNightBrightness, true);
  if (sSettings.kbdDayBrightness >= 10)
    disp->setKbdDayBrightness(sSettings.kbdDayBrightness, true);
  if (sSettings.kbdNightBrightness >= 10)
    disp->setKbdNightBrightness(sSettings.kbdNightBrightness, true);

  if (wifi) {
    wifi->enableMqtt(sSettings.mqttEnabled);
    wifi->mqttSetBroker(sSettings.mqttBroker);
    wifi->mqttSetPort(sSettings.mqttPort);
    wifi->mqttSetUser(sSettings.mqttUser);
    wifi->mqttSetPassword(sSettings.mqttPassword);
    wifi->mqttSetClientID(sSettings.mqttClientId);
    wifi->mqttSaveCredentialsOnConnect();
    wifi->setupMqttBroker();

    wifi->enableNtp(sSettings.ntpEnabled);
    wifi->ntpSetDisplayMode(sSettings.ntpDisplayMode);
    wifi->ntpSetServer(sSettings.ntpServer);
    wifi->ntpSetTimeZone(sSettings.timezone);
    wifi->ntpSaveCredentials();
    wifi->setupNtp();

    wifi->enableFtp(sSettings.ftpEnabled);
    wifi->mDNSSetName(sSettings.ftpMdnsName);
    wifi->ftpSetUser(sSettings.ftpUser);
    wifi->ftpSetPassword(sSettings.ftpPassword);
    wifi->ftpSaveCredentials();
  }

  hw.refreshImuMotionConfig();
}

void syncFromHardware() {
  auto &hw = HardwareFactory::getAbstract();
  sSettings.motionWakeEnabled = hw.getWakeupByIMUEnabled();
  sSettings.lightSleepEnabled = hw.getLightSleepEnabled();
  sSettings.lightSleepTimeoutMs = hw.getLightSleepTimeout();
  sSettings.displayTimeoutMs = hw.getSleepTimeout();

  auto disp = std::static_pointer_cast<Display>(hw.display());
  if (disp) {
    sSettings.lcdDayBrightness = disp->getLcdDayBrightness();
    sSettings.lcdNightBrightness = disp->getLcdNightBrightness();
    sSettings.kbdDayBrightness = disp->getKbdDayBrightness();
    sSettings.kbdNightBrightness = disp->getKbdNightBrightness();
  }
  auto wifi = hw.wifi();
  if (wifi) {
    sSettings.mqttEnabled = wifi->isMqttEnabled();
    sSettings.mqttBroker = wifi->mqttGetBroker();
    sSettings.mqttPort = wifi->mqttGetPort();
    sSettings.mqttUser = wifi->mqttGetUser();
    sSettings.mqttPassword = wifi->mqttGetPassword();
    sSettings.mqttClientId = wifi->mqttGetClientID();

    sSettings.ntpEnabled = wifi->isNtpEnabled();
    sSettings.ntpDisplayMode = wifi->ntpGetDisplayMode();
    sSettings.ntpServer = wifi->ntpGetServer();
    sSettings.timezone = wifi->ntpGetTimeZone();

    sSettings.ftpEnabled = wifi->isFtpEnabled();
    sSettings.ftpMdnsName = wifi->mDNSGetName();
    sSettings.ftpUser = wifi->ftpGetUser();
    sSettings.ftpPassword = wifi->ftpGetPassword();
  }
  clampDeepSleep();
}

void notifyActivity() {
  sLastActivityMs = millis();
  if (sScreenPoweredOff) {
    sScreenPoweredOff = false;
    if (auto disp = std::static_pointer_cast<Display>(HardwareFactory::getAbstract().display())) {
      disp->wake();
      disp->startFade(0);
    }
  } else if (auto disp = std::static_pointer_cast<Display>(HardwareFactory::getAbstract().display())) {
    if (disp->isPreSleepDim())
      disp->wake();
  }
}

uint32_t idleMs() {
  const uint32_t now = millis();
  return (now >= sLastActivityMs) ? (now - sLastActivityMs) : 0;
}

bool isScreenPoweredOff() { return sScreenPoweredOff; }

void setScreenPoweredOff(bool off) { sScreenPoweredOff = off; }

} // namespace device_settings
