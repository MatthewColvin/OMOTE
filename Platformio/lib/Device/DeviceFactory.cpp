#include "DeviceFactory.hpp"
#include <algorithm>
#include <cctype>

IDevice::Ptr DeviceFactory::Create(DeviceId id) {
  return nullptr;
}

void DeviceFactory::InitHomeAssistFactory(HomeAssist::WebSocket::Api &aHaApi) {
  mHomeAssistFactory = std::make_unique<HomeAssist::HomeAssistDeviceFactory>(aHaApi);
}

IDevice::Ptr DeviceFactory::CreateHomeAssistDevice(const std::string &aEntityString) {
  if (!mHomeAssistFactory) {
    return nullptr;
  }
  return mHomeAssistFactory->Create(aEntityString);
}
