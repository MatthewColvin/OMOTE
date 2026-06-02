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
  /** Reserved for editor / future NTP–MQTT wiring (stored in JSON only for now). */
  std::string ntpServer;
  std::string timezone;
};

Settings &current();
const Settings &currentConst();

bool loadFromLittleFS();
bool saveToLittleFS();
/** Merge keys from a JSON object; returns false if not an object. */
bool mergeFromJson(const rapidjson::Value &doc);
void applyToHardware();
void syncFromHardware();
void notifyActivity();

uint32_t idleMs();
bool isScreenPoweredOff();
void setScreenPoweredOff(bool off);

} // namespace device_settings
