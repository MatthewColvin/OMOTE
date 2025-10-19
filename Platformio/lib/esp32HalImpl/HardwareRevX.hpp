#pragma once
#include <Arduino.h>
#include <IRutils.h>
#include <Preferences.h>
#include <PubSubClient.h>

#include <functional>
#include <memory>

#include "Esp32Logger.hpp"
#include "EspStats.hpp"
#include "HardwareAbstract.hpp"
#include "IRTransceiver.hpp"
#include "LIS3DH_IMU.hpp"
#include "SparkFunLIS3DH.h"
#include "display.hpp"
#include "keys.hpp"
#include "lvgl.h"
#include "omoteconfig.h"
#include "wifihandler.hpp"
#if defined(OMOTE_HARDWARE_REV5)
#include <Adafruit_TCA8418.h>
#if defined(OMOTE_KEYBRD_3661)
#include <Adafruit_LTR329_LTR303.h>

#include "Panel_ST7789_NHD.h"
#include "Touch_FT5x26.h"
#endif
#endif

#define OBSERVER_BUF_SIZE 10

class HardwareRevX : public HardwareAbstract {
public:
  HardwareRevX();

  // HardwareAbstract
  virtual void init() override;
  virtual void debugPrint(const char *fmt, ...) override;

  virtual std::unique_ptr<LoggingInterface> logger() override;
  virtual std::shared_ptr<BatteryInterface> battery() override = 0;
  virtual std::shared_ptr<DisplayAbstract> display() override;
  virtual std::shared_ptr<wifiHandlerInterface> wifi() override;
  virtual std::shared_ptr<KeyPressAbstract> keys() override;
  virtual std::shared_ptr<IRInterface> ir() override;
  virtual std::shared_ptr<SystemStatsInterface> stats() override;
  virtual std::shared_ptr<webSocketInterface> webSocket() override;
  virtual std::shared_ptr<LIS3DH_IMU> imu();

  virtual std::chrono::milliseconds execTime() override;

  virtual char getCurrentDevice() override;
  virtual void setCurrentDevice(char currentDevice) override;

  virtual bool getWakeupByIMUEnabled() override;
  virtual void setWakeupByIMUEnabled(bool wakeupByIMUEnabled) override;

  virtual uint32_t getSleepTimeout() override;
  virtual void setSleepTimeout(uint32_t sleepTimeout) override;

  virtual bool getLightSleepEnabled() override;
  virtual void setLightSleepEnabled(bool wakeupByIMUEnabled) override;

  virtual uint32_t getLightSleepTimeout() override;
  virtual void setLightSleepTimeout(uint32_t sleepTimeout) override;

  virtual void saveSettings() override;

  virtual void setInScene(bool inScene) override;

  /// @brief To be ran in loop out in main
  void loopHandler() override;

  virtual void enterSleep(SleepMode mode = SleepMode::LIGHT_DEEP_SLEEP, uint32_t duration = 0) override;
  WakeReason getWakeUpReason() override { return mWakeupReason; };
  inline unsigned long getMillis() override { return millis(); };
  virtual bool isUsbConnected() override { return false; };
  unsigned long getWakeTime() override { return mWakeTime; };

protected:
  // Init Functions to setup hardware
  virtual void initIO();
  void restorePreferences();

  virtual bool keyboardScan() = 0;
  virtual bool lightSensorScan(uint16_t &visPlusIrLevel, uint16_t &irLevel);
  virtual void updateBacklightMode(uint16_t lightLevel);

  // void enterSleep(SleepMode mode);
  virtual void lightSleepWakeReint(SleepMode mode);
  virtual void enableWakeupByPin();
  virtual void sleepDisplayPins() = 0;
  virtual void configPinsForSleepInterrupts() {};

  // Tasks
  void startTasks();

  // Maybe TODO: make not protected?
protected:
  std::shared_ptr<Keys> mKeys;
  std::shared_ptr<Display> mDisplay;

private:
  std::shared_ptr<wifiHandler> mWifiHandler;
  std::shared_ptr<IRTransceiver> mIr;
  std::shared_ptr<EspStats> mStats = nullptr;

protected: // Maybe todo: make private?
  // IMU Motion Detection
  LIS3DH mIMU =
      LIS3DH(I2C_MODE, 0x19); // Default constructor is I2C, addr 0x19.
  std::shared_ptr<LIS3DH_IMU> mIMU_new = nullptr;
  Preferences mPreferences;

private:
  int mStandbyTimer = SLEEP_TIMEOUT;
  int mSleepTimeout = SLEEP_TIMEOUT;
  uint32_t mLightSleepTimeout = LIGHT_SLEEP_TIMEOUT;
  unsigned long mIMUTaskTimer = 0;
  int mMotion = 0;
  WakeReason mWakeupReason;
  bool mInScene = false;
  // ESP32Logger mLogger;

  unsigned long mWakeTime = 0;

  bool mWakeupByIMUEnabled = true;
  bool mLightSleepEnabled = false;
  byte mCurrentDevice = 1; // Current Device to control (allows switching
                           // mappings between devices)

  Handler<Display::TouchPointType> mTouchHandler;

  // std::unique_ptr<LoggingInterface> mLogger;
  std::unique_ptr<LoggingInterface> mLogger = nullptr;
  mutable std::stringstream mLogStream;
};
