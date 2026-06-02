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
  if (doc.HasMember("ntp_server") && doc["ntp_server"].IsString())
    sSettings.ntpServer = doc["ntp_server"].GetString();
  if (doc.HasMember("timezone") && doc["timezone"].IsString())
    sSettings.timezone = doc["timezone"].GetString();

  clampDeepSleep();
  return true;
}

bool loadFromLittleFS() {
  const auto doc = OMOTE::JSON::GetDocument(std::filesystem::path(FS_PATH "DeviceSettings.json"));
  if (doc.HasParseError() || !doc.IsObject())
    return false;
  return mergeFromJson(doc);
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
  if (!sSettings.ntpServer.empty())
    d.AddMember("ntp_server", rapidjson::Value(sSettings.ntpServer.c_str(), a), a);
  if (!sSettings.timezone.empty())
    d.AddMember("timezone", rapidjson::Value(sSettings.timezone.c_str(), a), a);

  std::ofstream out(vfsPath("DeviceSettings.json"), std::ios::out | std::ios::trunc);
  if (!out)
    return false;
  const std::string body = OMOTE::JSON::ToString(d);
  out << body;
  return true;
}

void applyToHardware() {
  auto &hw = HardwareFactory::getAbstract();
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
