#include <chrono>
#include <cmath>

#include "Hardware/BatteryInterface.h"
#include "observerHandles.hpp"

#define OBS_BUF_SIZE 10

class BatterySimulator : public BatteryInterface {
public:
  BatterySimulator()
      : mCreationTime(std::chrono::high_resolution_clock::now()) {
    UI::observerHandles::registerTextHandle(BATT_STATUS, OBS_BUF_SIZE, "");
    UI::observerHandles::registerTextHandle(WIFI_STATUS, OBS_BUF_SIZE, "");
    UI::observerHandles::registerIntHandle(SOC_STATUS, 0);
  };
  ~BatterySimulator() {}

  virtual int getPercentage() override {
    auto now = std::chrono::high_resolution_clock::now();
    auto batteryRunTime =
        std::chrono::duration_cast<std::chrono::seconds>(now - mCreationTime);
    constexpr auto minToBatteryZero = 3;
    auto fakeBattPercentage =
        100 - ((batteryRunTime / std::chrono::duration<float, std::ratio<60LL>>(
                                     minToBatteryZero)) *
               100);
    if (fakeBattPercentage < 0)
      fakeBattPercentage = 0;

    int32_t iSoc = fakeBattPercentage;
    if (iSoc > 99)
      iSoc = 99;
    UI::observerHandles::setInt(SOC_STATUS, iSoc);
    if (iSoc < 13)
      UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_EMPTY);
    else if (iSoc < 38)
      UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_1);
    else if (iSoc < 63)
      UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_2);
    else if (iSoc < 88)
      UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_3);
    else
      UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_FULL);

    return std::floor(fakeBattPercentage < 100 ? fakeBattPercentage : 0);
  }

  virtual bool isCharging() override { return false; }

private:
  std::chrono::_V2::system_clock::time_point mCreationTime;
};