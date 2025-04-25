#include "HardwareSimulator.hpp"
#include "littlefs/LittlefsSim.hpp"

#include <sstream>

HardwareSimulator::HardwareSimulator()
    : HardwareAbstract(),
      mLittleFsSim(LittlefsSim::getInstance()),
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
  mLittleFsSim->mount();
#ifdef INIT_LITTLEFS_FROM_DATA
  mLittleFsSim->initFolderContents("./data");
#endif

  mSDLEventHandler.SetNotification(mKeys->getSDLEventNotification());
  mSDLEventHandler = [this](SDL_Event *aEvent) { handleExtraSDLEvents(aEvent); };
}

std::shared_ptr<LittleFsInterface> HardwareSimulator::littleFs() {
  return mLittleFsSim;
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

bool HardwareSimulator::getWakeupByIMUEnabled() { return true; }

void HardwareSimulator::setWakeupByIMUEnabled(bool wakeupByIMUEnabled) {}

uint32_t HardwareSimulator::getSleepTimeout() { return 20000; }

void HardwareSimulator::setSleepTimeout(uint32_t sleepTimeout) {}

void HardwareSimulator::handleExtraSDLEvents(SDL_Event *aEvent) {
  if (aEvent->type == SDL_KEYDOWN) {
    const auto SDLK_key = aEvent->key.keysym.sym;
    if (SDLK_key == SDLK_F1) {
      mLittleFsSim->dumpContentsToFolder("./data_backup");
    } else if (SDLK_key == SDLK_F2) {
      mLittleFsSim->initFolderContents("./data");
    }
  }
}
