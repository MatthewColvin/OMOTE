// OMOTE Hardware Abstraction
// 2023 Matthew Colvin
#pragma once
#include <chrono>
#include <memory>

#include "Hardware/BatteryInterface.h"
#include "Hardware/DisplayAbstract.h"
#include "Hardware/IRInterface.h"
#include "Hardware/KeyPressAbstract.hpp"
#include "Hardware/LoggingInterface.hpp"
#include "Hardware/SystemStatsInterface.h"
#include "Hardware/wifi/websockets/webSocketInterface.hpp"
#include "Hardware/wifi/wifiHandlerInterface.h"
#include "Notification.hpp"

class HardwareAbstract {
public:
  enum class WakeReason { RESET,
                          TIMER,
                          CHARGER,
                          IMU,
                          KEYPAD };

  enum class SleepMode { LIGHT_SLEEP_WAKE_ON_CHG,   // light then continue
                         LIGHT_SLEEP_WAKE_ON_NOCHG, // light then continue
                         LIGHT_DEEP_SLEEP,          // light then deep
                         DEEP_SLEEP };

  HardwareAbstract() = default;
  virtual ~HardwareAbstract() = default;

  /// @brief Override in order to do setup of hardware devices post construction
  virtual void init() = 0;

  /// @brief Override to do processing in main thread
  virtual void loopHandler() = 0;

  /// @brief Override to allow printing of a message for debugging
  /// @param message - Debug message
  virtual void debugPrint(const char *fmt, ...) = 0;

  // TODO: Evaluate if worth having a common interface for all hardware modules
  // so that they can easily do stuff in the main loop function or in hardware in a common way.
  // con would be that the logic that ends up there might be cause an issue and hard to see why.
  virtual std::unique_ptr<LoggingInterface> logger() = 0;
  virtual std::shared_ptr<BatteryInterface> battery() = 0;
  virtual std::shared_ptr<DisplayAbstract> display() = 0;
  virtual std::shared_ptr<wifiHandlerInterface> wifi() = 0;
  virtual std::shared_ptr<KeyPressAbstract> keys() = 0;
  virtual std::shared_ptr<IRInterface> ir() = 0;
  virtual std::shared_ptr<SystemStatsInterface> stats() = 0;
  virtual std::shared_ptr<webSocketInterface> webSocket() = 0;

  virtual std::chrono::milliseconds execTime() = 0;

  virtual char getCurrentDevice() = 0;
  virtual void setCurrentDevice(char currentDevice) = 0;

  virtual bool getWakeupByIMUEnabled() = 0;
  virtual void setWakeupByIMUEnabled(bool wakeupByIMUEnabled) = 0;

  virtual uint32_t getSleepTimeout() = 0;
  virtual void setSleepTimeout(uint32_t sleepTimeout) = 0;

  virtual bool getLightSleepEnabled() = 0;
  virtual void setLightSleepEnabled(bool wakeupByIMUEnabled) = 0;

  virtual uint32_t getLightSleepTimeout() = 0;
  virtual void setLightSleepTimeout(uint32_t sleepTimeout) = 0;

  virtual void saveSettings() = 0;

  // TODO: Scenes are really a UI Structure
  // Lets try to refactor this out of HAL into some sort of UI structure.
  virtual void setInScene(bool inScene) = 0;

  virtual void enterSleep(SleepMode mode, uint32_t duration) = 0;
  virtual WakeReason getWakeUpReason() = 0;
  virtual unsigned long getMillis() = 0;
  virtual bool isUsbConnected() = 0;
  virtual unsigned long getWakeTime() = 0;
};
