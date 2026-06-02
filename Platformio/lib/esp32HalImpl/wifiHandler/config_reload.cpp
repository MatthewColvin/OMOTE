#include "config_reload.hpp"

namespace {

volatile bool sPagesDirty = false;
volatile bool sHaSettingsDirty = false;
volatile bool sDeviceSettingsDirty = false;
volatile bool sDeviceSettingsSchemaDirty = false;

} // namespace

namespace config_reload {

void markPagesDirty() { sPagesDirty = true; }

void markHaSettingsDirty() { sHaSettingsDirty = true; }

void markDeviceSettingsDirty() { sDeviceSettingsDirty = true; }

void markDeviceSettingsSchemaDirty() { sDeviceSettingsSchemaDirty = true; }

bool consumePagesDirty() {
  if (!sPagesDirty)
    return false;
  sPagesDirty = false;
  return true;
}

bool consumeHaSettingsDirty() {
  if (!sHaSettingsDirty)
    return false;
  sHaSettingsDirty = false;
  return true;
}

bool consumeDeviceSettingsDirty() {
  if (!sDeviceSettingsDirty)
    return false;
  sDeviceSettingsDirty = false;
  return true;
}

bool consumeDeviceSettingsSchemaDirty() {
  if (!sDeviceSettingsSchemaDirty)
    return false;
  sDeviceSettingsSchemaDirty = false;
  return true;
}

} // namespace config_reload
