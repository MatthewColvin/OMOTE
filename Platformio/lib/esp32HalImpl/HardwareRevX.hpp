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

  virtual uint16_t getSleepTimeout() override;
  virtual void setSleepTimeout(uint16_t sleepTimeout) override;

  /// @brief To be ran in loop out in main
  // TODO move to a freertos task
  void loopHandler() override;

 protected:
  // Init Functions to setup hardware
  virtual void initIO();
  void restorePreferences();
  void setupIMU();
#if defined(OMOTE_HARDWARE_REV5)
  void setupKeyboard();
#endif

  void activityDetection();
  void keyboardScan();
  void enterSleep();
  void configIMUInterrupts();

  // Tasks
  void startTasks();

 private:
  std::shared_ptr<Battery> mBattery;
  std::shared_ptr<Display> mDisplay;
  std::shared_ptr<wifiHandler> mWifiHandler;
  std::shared_ptr<Keys> mKeys;
  std::shared_ptr<IRTransceiver> mIr;
  std::shared_ptr<EspStats> mStats = nullptr;

#if defined(OMOTE_HARDWARE_REV5)
  //  Battery gas gauge
  SFE_MAX1704X fuelGauge = SFE_MAX1704X(MAX1704X_MAX17048);
  // keypad scanning
  Adafruit_TCA8418 keypad;
#if defined(OMOTE_KEYBRD_3661)
  // light sensor
  Adafruit_LTR303 ltr = Adafruit_LTR303();
#endif
#endif
  // IMU Motion Detection
  LIS3DH IMU =
      LIS3DH(I2C_MODE, 0x19);  // Default constructor is I2C, addr 0x19.
  int standbyTimer = SLEEP_TIMEOUT;
  int sleepTimeout = SLEEP_TIMEOUT;
  int motion = 0;
  WakeReason wakeup_reason;

  Preferences preferences;
  bool wakeupByIMUEnabled = true;
  byte currentDevice = 1;  // Current Device to control (allows switching
                           // mappings between devices)

  static std::shared_ptr<HardwareRevX> mInstance;
  Handler<Display::TouchPointType> mTouchHandler;
#if defined(OMOTE_HARDWARE_REV5)
  QueueHandle_t mKeysQueueHandle;
  char indexToChar[KEYPAD_ROWS * KEYPAD_COLS] = {
      '+', '-', 'i',
      'L', 'b', 'o',  // volume+, volume-,    info,    left,  back,  NotUsed
      't', 'm', 'k',
      'h', '<', '=',  //  return,    mute,      OK,    home,  rewind,  stop,
      '^', 'g', 'd',
      'p', 's', 'T',  // channel+,   guide,    down,    play,   pause,  TV
      'v', 'u', 'x',
      'r', 'S', 'A',  // channel-,      up,    exit,  record,  stream,  audio
      'c', 'R', '>',
      'B', 'D', 'Y'};  //    config,   right, forward,     STB,     DVD,  BLURAY
#endif
};