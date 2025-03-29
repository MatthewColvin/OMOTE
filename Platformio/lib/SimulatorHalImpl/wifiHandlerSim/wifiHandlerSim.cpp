#include "wifiHandlerSim.hpp"

using WifiInfo = wifiHandlerInterface::WifiInfo;

wifiHandlerSim::wifiHandlerSim() {}

void wifiHandlerSim::begin() {}

void wifiHandlerSim::connect(std::string ssid, std::string password) {
  if (mFakeStatusThread.joinable()) {
    mFakeStatusThread.join();
    mCurrentStatus.ssid = ssid;
    mCurrentStatus.isConnected = true;
    mFakeStatusThread = std::thread([this] {
      std::this_thread::sleep_for(std::chrono::seconds(1));
      mStatusUpdate->notify(mCurrentStatus);
    });
  }
}

static const WifiInfo wifis[] = {
    WifiInfo("High Signal Wifi", -49, false), WifiInfo("Mid Signal Wifi", -55, true),
    WifiInfo("Low Signal Wifi", -65, false), WifiInfo("No Signal Wifi", -90, false)};

void wifiHandlerSim::scan() {
  if (mFakeScanThread.joinable()) {
    mFakeScanThread.join();
    mFakeScanThread = std::thread([this] {
      std::vector<WifiInfo> info =
          std::vector(std::begin(wifis), std::end(wifis));
      std::this_thread::sleep_for(std::chrono::seconds(2));
      mScanNotification->notify(info);
    });
  }
}
