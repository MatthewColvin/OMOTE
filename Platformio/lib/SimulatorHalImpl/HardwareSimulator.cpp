#include "HardwareSimulator.hpp"
#include <filesystem>
#include <sstream>

HardwareSimulator::HardwareSimulator()
    : HardwareAbstract(),
      mBattery(std::make_shared<BatterySimulator>()),
      mDisplay(SDLDisplay::getInstance()),
      mWifiHandler(std::make_shared<wifiHandlerSim>()),
      mKeys(std::make_shared<KeyPressSim>()),
      mIr(std::make_shared<IRSim>()),
      mStats(std::make_shared<StatsSimulator>()),
      mStartTime(std::chrono::high_resolution_clock::now()) {
  mHardwareStatusTitleUpdate = std::thread([this] {
    int dataToShow = 0;
    while (true) {
      std::stringstream title;
      switch (dataToShow) {
      case 0:
        // title << "Batt:" << mBattery->getPercentage() << "%" << std::endl;
        // dataToShow = -1;
        break;
      case 1:
        // title << "BKLght: " << static_cast<int>(mDisplay->getBrightness())
        //       << std::endl;
        dataToShow = -1;
        break;
      default:
        dataToShow = -1;
      }
      dataToShow++;

      mDisplay->setTitle(title.str());
      std::this_thread::sleep_for(std::chrono::seconds(2));
    }
  });
#ifdef INIT_SIM_DATA_FROM_DATA
  initDirectory("./sim_data", "./data");
#endif

  mSDLEventHandler.SetNotification(mKeys->getSDLEventNotification());
  mSDLEventHandler = [this](SDL_Event *aEvent) { handleExtraSDLEvents(aEvent); };
}

void HardwareSimulator::init() {
  LoggingInterface::restoreSettings();
}

void HardwareSimulator::loopHandler() {
  static auto oldTime = std::chrono::high_resolution_clock::now();

  auto now = std::chrono::high_resolution_clock::now();
  if (std::chrono::duration_cast<std::chrono::milliseconds>(now - oldTime) > std::chrono::milliseconds(25)) {
    mBattery->getPercentage();
    mWifiHandler->mqttSync();
    mKeys->KeyboardScan();
    oldTime = std::chrono::high_resolution_clock::now();
  }
}

unsigned long HardwareSimulator::getMillis() {
  static auto oldTime = std::chrono::high_resolution_clock::now();

  auto now = std::chrono::high_resolution_clock::now();
  auto msec = std::chrono::duration_cast<std::chrono::milliseconds>(now - oldTime);

  return msec.count();
}

bool isUsbConnected() { return false; }
unsigned long getWakeTime() { return 0; }

std::unique_ptr<LoggingInterface> HardwareSimulator::logger() {
  return std::make_unique<SimLogger>();
}

std::shared_ptr<BatteryInterface> HardwareSimulator::battery() {
  return mBattery;
}
std::shared_ptr<DisplayAbstract> HardwareSimulator::display() {
  return mDisplay;
}
std::shared_ptr<wifiHandlerInterface> HardwareSimulator::wifi() {
  return mWifiHandler;
}
std::shared_ptr<KeyPressAbstract> HardwareSimulator::keys() { return mKeys; }

std::shared_ptr<IRInterface> HardwareSimulator::ir() { return mIr; }

std::shared_ptr<SystemStatsInterface> HardwareSimulator::stats() {
  return mStats;
}

std::shared_ptr<webSocketInterface> HardwareSimulator::webSocket() {
  for (auto &socket : mWebSockets) {
    if (socket.expired()) {
      auto newsocket = std::make_shared<webSocketSimulator>();
      socket = newsocket;
      return newsocket;
    }
  }
  return nullptr;
}

std::chrono::milliseconds HardwareSimulator::execTime() {
  auto now = std::chrono::high_resolution_clock::now();
  auto duration =
      std::chrono::duration_cast<std::chrono::milliseconds>(now - mStartTime);
  return duration;
}

char HardwareSimulator::getCurrentDevice() { return 0; }

void HardwareSimulator::setCurrentDevice(char currentDevice) {}

bool HardwareSimulator::getWakeupByIMUEnabled() { return mImuWakeEn; }

void HardwareSimulator::setWakeupByIMUEnabled(bool wakeupByIMUEnabled) { mImuWakeEn = wakeupByIMUEnabled; }

uint32_t HardwareSimulator::getSleepTimeout() { return mSleepTimeout; }

void HardwareSimulator::setSleepTimeout(uint32_t sleepTimeout) { mSleepTimeout = sleepTimeout; }

bool HardwareSimulator::getLightSleepEnabled() { return mLightSleepEn; };
void HardwareSimulator::setLightSleepEnabled(bool LightSlpEnabled) { mLightSleepEn = LightSlpEnabled; };

uint32_t HardwareSimulator::getLightSleepTimeout() { return mLightSlpTimeout; };
void HardwareSimulator::setLightSleepTimeout(uint32_t sleepTimeout) { mLightSlpTimeout = sleepTimeout; };

void HardwareSimulator::handleExtraSDLEvents(SDL_Event *aEvent) {
  if (aEvent->type == SDL_KEYDOWN) {
    const auto SDLK_key = aEvent->key.keysym.sym;
    if (SDLK_key == SDLK_F1) {
      dumpDirectory("./sim_data", "./data_backup");
    } else if (SDLK_key == SDLK_F2) {
      initDirectory("./sim_data", "./data");
    }
  }
}

bool HardwareSimulator::dumpDirectory(const char *path, const std::string &outputPath) {
  std::filesystem::copy(path, outputPath, std::filesystem::copy_options::recursive);
  return true;
}

bool HardwareSimulator::initDirectory(const char *path, const std::string &inputPath) {
  std::filesystem::remove_all(path);
  std::filesystem::copy(inputPath, path, std::filesystem::copy_options::recursive);
  return true;
}
