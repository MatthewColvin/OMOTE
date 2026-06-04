#include "editor_sync_mode.hpp"

#include "HardwareFactory.hpp"

namespace {

bool sActive = false;
bool sShowOverlay = false;
uint32_t sSavedSleepTimeout = 0;
uint32_t sSavedLightSleepTimeout = 0;
bool sSavedLightSleepEnabled = false;

} // namespace

namespace editor_sync_mode {

bool isActive() { return sActive; }

bool overlayRequested() { return false; }

bool enter(bool showOverlay) {
  (void)showOverlay;
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

  if (auto ir = hw.ir())
    ir->disableRx();

  return true;
}

void exit(bool /*reboot*/) {
  if (!sActive)
    return;

  sActive = false;
  auto &hw = HardwareFactory::getAbstract();
  hw.setSleepTimeout(sSavedSleepTimeout ? sSavedSleepTimeout : 20000);
  hw.setLightSleepTimeout(sSavedLightSleepTimeout ? sSavedLightSleepTimeout : 60000);
  hw.setLightSleepEnabled(sSavedLightSleepEnabled);

  if (auto ir = hw.ir())
    ir->enableRx();
}

} // namespace editor_sync_mode
