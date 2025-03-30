#pragma once

#include "DeviceIds.hpp"

#include "HomeAssistDevices/HomeAssistDeviceFactory.hpp"
#include "IDevice.hpp"

#include <memory>
#include <string>

class DeviceFactory {
public:
  static IDevice::Ptr Create(DeviceId aId);
  static IDevice::Ptr CreateHomeAssistDevice(const std::string &aEntityString, HomeAssist::WebSocket::Api &mHaApi);

private:
  DeviceFactory() = default;
};
