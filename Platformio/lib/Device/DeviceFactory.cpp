#include "DeviceFactory.hpp"
#include "ActiveDeviceConfig.hpp"
#include "HardwareFactory.hpp"
#include <algorithm>
#include <cctype>

DeviceFactory::DeviceFactory() {
  mDeviceConfig = std::make_unique<ActiveDeviceConfig>(HardwareFactory::getAbstract().littleFs(), *this);
}

IDevice::Ptr DeviceFactory::Create(DeviceId id) {
  return nullptr;
}

void DeviceFactory::InitHomeAssistFactory(HomeAssist::WebSocket::Api &aHaApi) {
  mHomeAssistFactory = std::make_unique<HomeAssist::HomeAssistDeviceFactory>(aHaApi);
}

IDevice::Ptr DeviceFactory::CreateHomeAssistDevice(const std::string &aEntityString) {
  HardwareFactory::getAbstract().debugPrint("Attempting Creation of %s", aEntityString.c_str());

  if (!mHomeAssistFactory) {
    return nullptr;
  }
  return mHomeAssistFactory->Create(aEntityString);
}

void DeviceFactory::restoreFromConfig() {
  auto devices = mDeviceConfig->loadDevices();
  mActiveDevices.restoreDevices(devices);
}
