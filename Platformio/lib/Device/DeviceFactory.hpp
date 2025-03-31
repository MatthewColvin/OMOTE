#pragma once

#include "DeviceIds.hpp"
#include "ActiveDevices.hpp"
#include "HomeAssistDevices/HomeAssistDeviceFactory.hpp"
#include "IDevice.hpp"

#include <memory>
#include <string>

class DeviceFactory {
public:
  DeviceFactory() = default;
  IDevice::Ptr Create(DeviceId aId);
  IDevice::Ptr CreateHomeAssistDevice(const std::string &aEntityString, HomeAssist::WebSocket::Api &mHaApi);
  
  ActiveDevices& getActiveDevices() { return mActiveDevices; }

private:
  ActiveDevices mActiveDevices;
};
