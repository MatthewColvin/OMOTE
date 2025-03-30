#include "DeviceFactory.hpp"
#include <algorithm>
#include <cctype>

IDevice::Ptr DeviceFactory::Create(DeviceId id) {
  return nullptr;
}

IDevice::Ptr DeviceFactory::CreateHomeAssistDevice(const std::string &aEntityString, HomeAssist::WebSocket::Api &mHaApi) {
  return HomeAssist::HomeAssistDeviceFactory::Create(aEntityString, mHaApi);
}
