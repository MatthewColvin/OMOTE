#include "DeviceFactory.hpp"
#include <algorithm>
#include <cctype>

IDevice::Ptr DeviceFactory::Create(DeviceId id) {
  return nullptr;
}

IDevice::Ptr DeviceFactory::CreateHomeAssistDevice(const std::string &aEntityString, HomeAssist::WebSocket::Api &mHaApi) {
  auto device = HomeAssist::HomeAssistDeviceFactory::Create(aEntityString, mHaApi);
  if (device) {
    mActiveDevices.addDevice(device);
  }
  return device;
}
