#include "config_reload.hpp"

namespace {

volatile bool sPagesDirty = false;
volatile bool sHaSettingsDirty = false;

} // namespace

namespace config_reload {

void markPagesDirty() { sPagesDirty = true; }

void markHaSettingsDirty() { sHaSettingsDirty = true; }

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

} // namespace config_reload
