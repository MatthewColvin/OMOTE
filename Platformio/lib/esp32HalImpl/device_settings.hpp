#pragma once

#include <cstdint>
#include <string>

#include <rapidjson/document.h>

namespace device_settings {

/** Runtime + LittleFS mirror of editor/on-device preferences. */
struct Settings {
  uint32_t displayTimeoutMs = 60000;
  uint32_t deepSleepTimeoutMs = 900000;
  uint32_t dimLeadMs = 2000;
  bool motionWakeEnabled = true;
  bool keyWakeEnabled = true;
  bool lightSleepEnabled = false;
  uint32_t lightSleepTimeoutMs = 60000;
  uint8_t lcdDayBrightness = 0;
  uint8_t lcdNightBrightness = 0;
  uint8_t kbdDayBrightness = 0;
  uint8_t kbdNightBrightness = 0;
  bool mqttEnabled = false;
  std::string mqttBroker = "broker";
  std::string mqttPort = "1883";
  std::string mqttUser = "user";
  std::string mqttPassword = "password";
  std::string mqttClientId = "OMOTE";
  bool ntpEnabled = false;
  int32_t ntpDisplayMode = 0;
  std::string ntpServer;
  std::string timezone;
  bool ftpEnabled = false;
  std::string ftpMdnsName = "omote";
  std::string ftpUser = "OMOTE";
  std::string ftpPassword = "OMOTE";
};

Settings &current();
const Settings &currentConst();

bool loadFromLittleFS();
bool saveToLittleFS();
rapidjson::Document toJsonDocument();
/** Merge keys from a JSON object; returns false if not an object. */
bool mergeFromJson(const rapidjson::Value &doc);
void applyToHardware();
void syncFromHardware();
void notifyActivity();

uint32_t idleMs();
bool isScreenPoweredOff();
void setScreenPoweredOff(bool off);

} // namespace device_settings
