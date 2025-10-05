#pragma once
#include <thread>

#include "HardwareAbstract.hpp"
#include "IRSim.hpp"
#include "KeyPressSim.hpp"
#include "SDLDisplay.hpp"
#include "StatsSimulator.hpp"
#include "batterySimulator.hpp"
#include "simLogger.hpp"
#include "webSocketSimulator.hpp"
#include "wifiHandlerSim.hpp"

class HardwareSimulator : public HardwareAbstract {
public:
  HardwareSimulator();

  void init() override;
  void loopHandler() override;

  void debugPrint(const char *fmt, ...) override {
    va_list arguments;
    va_start(arguments, fmt);
    vprintf(fmt, arguments);
    va_end(arguments);
    fflush(stdout);
  }

  std::unique_ptr<LoggingInterface> logger() override;
  std::shared_ptr<BatteryInterface> battery() override;
  std::shared_ptr<DisplayAbstract> display() override;
  std::shared_ptr<wifiHandlerInterface> wifi() override;
  std::shared_ptr<KeyPressAbstract> keys() override;
  std::shared_ptr<IRInterface> ir() override;
  std::shared_ptr<SystemStatsInterface> stats() override;
  std::shared_ptr<webSocketInterface> webSocket() override;

  std::chrono::milliseconds execTime() override;

  char getCurrentDevice() override;
  void setCurrentDevice(char currentDevice) override;

  bool getWakeupByIMUEnabled() override;
  void setWakeupByIMUEnabled(bool wakeupByIMUEnabled) override;

  uint32_t getSleepTimeout() override;
  void setSleepTimeout(uint32_t sleepTimeout) override;

  bool getLightSleepEnabled() override;
  void setLightSleepEnabled(bool LightSlpEnabled) override;

  uint32_t getLightSleepTimeout() override;
  void setLightSleepTimeout(uint32_t sleepTimeout) override;

  void saveSettings() override {};

  void enterSleep(SleepMode mode, uint32_t duration) override {};
  WakeReason getWakeUpReason() override { return HardwareAbstract::WakeReason::TIMER; };
  unsigned long getMillis() override;
protected:
  void handleExtraSDLEvents(SDL_Event *aEvent);
  bool initDirectory(const char *path, const std::string &inputPath);
  bool dumpDirectory(const char *path, const std::string &outputPath);

private:
  // Completely arbitrary limit on the number of web sockets
  static constexpr auto WebSocketLimit = 5;

  std::thread mHardwareStatusTitleUpdate;
  std::thread mMqttUpdate;

  std::shared_ptr<BatterySimulator> mBattery;
  std::shared_ptr<SDLDisplay> mDisplay;
  std::shared_ptr<wifiHandlerSim> mWifiHandler;
  std::shared_ptr<KeyPressSim> mKeys;
  std::shared_ptr<IRSim> mIr;
  std::shared_ptr<StatsSimulator> mStats;
  std::array<std::weak_ptr<webSocketSimulator>, WebSocketLimit> mWebSockets;

  std::chrono::system_clock::time_point mStartTime;

  Handler<SDL_Event *> mSDLEventHandler;
  bool mLightSleepEn = true;
  uint32_t mSleepTimeout = 10000;
  uint32_t mLightSlpTimeout = 60000;
};
