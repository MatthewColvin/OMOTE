#include "editor_sync_mode.hpp"

#include "HardwareFactory.hpp"
#include "ir/IRTransceiver.hpp"

#include <ESP.h>
#include <WiFi.h>

namespace {

bool sActive = false;
uint32_t sSavedSleepTimeout = 0;
uint32_t sSavedLightSleepTimeout = 0;
bool sSavedLightSleepEnabled = false;

} // namespace

namespace editor_sync_mode {

bool isActive() { return sActive; }

bool enter() {
  if (sActive)
    return true;

  sActive = true;
  auto &hw = HardwareFactory::getAbstract();
  sSavedSleepTimeout = hw.getSleepTimeout();
  sSavedLightSleepTimeout = hw.getLightSleepTimeout();
  sSavedLightSleepEnabled = hw.getLightSleepEnabled();

  hw.setSleepTimeout(30UL * 60UL * 1000UL);
  hw.setLightSleepTimeout(0);
  hw.setLightSleepEnabled(false);

  if (auto *ir = static_cast<IRTransceiver *>(hw.ir().get()))
    ir->disableRx();

  return true;
}

void exit(bool reboot) {
  if (!sActive) {
    if (reboot)
      ESP.restart();
    return;
  }

  sActive = false;
  auto &hw = HardwareFactory::getAbstract();
  hw.setSleepTimeout(sSavedSleepTimeout ? sSavedSleepTimeout : 20000);
  hw.setLightSleepTimeout(sSavedLightSleepTimeout ? sSavedLightSleepTimeout : 60000);
  hw.setLightSleepEnabled(sSavedLightSleepEnabled);

  if (reboot) {
    delay(80);
    ESP.restart();
  }
}

} // namespace editor_sync_mode
