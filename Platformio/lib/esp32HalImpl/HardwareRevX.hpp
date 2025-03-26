#pragma once
#include <Arduino.h>
#include <IRutils.h>
#include <Preferences.h>
#include <PubSubClient.h>

#include <functional>
#include <memory>

#include "EspStats.hpp"
#include "HardwareAbstract.hpp"
#include "IRTransceiver.hpp"
#include "SparkFunLIS3DH.h"
#include "battery.hpp"
#include "display.hpp"
#include "keys.hpp"
#include "lvgl.h"
#include "omoteconfig.h"
#include "wifihandler.hpp"
#if defined(OMOTE_HARDWARE_REV5)
#include <Adafruit_TCA8418.h>
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>
#if defined(OMOTE_KEYBRD_3661)
#include <Adafruit_LTR329_LTR303.h>

#include "Panel_ST7789_NHD.h"
#include "Touch_FT5x26.h"
#endif
#endif

class HardwareRevX : public HardwareAbstract {
 public:
  enum class WakeReason { RESET, IMU, KEYPAD };

  HardwareRevX();

  // HardwareAbstract
  virtual void init() override;
  virtual void debugPrint(const char *fmt, ...) override;

  virtual std::shared_ptr<BatteryInterface> battery() override;
  virtual std::shared_ptr<DisplayAbstract> display() override;
  virtual std::shared_ptr<wifiHandlerInterface> wifi() override;
  virtual std::shared_ptr<KeyPressAbstract> keys() override;
  virtual std::shared_ptr<IRInterface> ir() override;
  virtual std::shared_ptr<SystemStatsInterface> stats() override;
  virtual std::shared_ptr<webSocketInterface> webSocket() override;

  virtual std::chrono::milliseconds execTime() override;

  virtual char getCurrentDevice() override;
  virtual void setCurrentDevice(char currentDevice) override;

  virtual bool getWakeupByIMUEnabled() override;
  virtual void setWakeupByIMUEnabled(bool wakeupByIMUEnabled) override;

  virtual uint32_t getSleepTimeout() override;
  virtual void setSleepTimeout(uint32_t sleepTimeout) override;

  /// @brief To be ran in loop out in main
  // TODO move to a freertos task
  void loopHandler() override;

 protected:
  // Init Functions to setup hardware
  virtual void initIO();
  void restorePreferences();
  void setupIMU();

  virtual bool keyboardScan() {return false;};
  virtual bool lightSensorScan(uint16_t &visPlusIrLevel, uint16_t &irLevel) {return false;};
  virtual void updateBacklightMode(uint16_t lightLevel) {};
  virtual bool fuelGaugeScan(float &soc, float &voltage) {return false;};
 
  bool activityDetection();
  void enterSleep();
  void configIMUInterrupts();
  virtual void configIMUInterruptPolarity();
  virtual void enableWakeupByPin();
  virtual void sleepDisplayPins() = 0;
  virtual void configPinsForSleepInterrupts() {};

  // Tasks
  void startTasks();

  // Maybe TODO: make not protected?
 protected:
  std::shared_ptr<Battery> mBattery;
  std::shared_ptr<Keys> mKeys;
  std::shared_ptr<Display> mDisplay;

 private:
  std::shared_ptr<wifiHandler> mWifiHandler;
  std::shared_ptr<IRTransceiver> mIr;
  std::shared_ptr<EspStats> mStats = nullptr;

 protected:  // Maybe todo: make private?
  // IMU Motion Detection
  LIS3DH IMU =
      LIS3DH(I2C_MODE, 0x19);  // Default constructor is I2C, addr 0x19.
  Preferences preferences;

 private:
  int standbyTimer = SLEEP_TIMEOUT;
  int sleepTimeout = SLEEP_TIMEOUT;
  int motion = 0;
  WakeReason wakeup_reason;

  bool wakeupByIMUEnabled = true;
  byte currentDevice = 1;  // Current Device to control (allows switching
                           // mappings between devices)

  static std::shared_ptr<HardwareRevX> mInstance;
  Handler<Display::TouchPointType> mTouchHandler;
};