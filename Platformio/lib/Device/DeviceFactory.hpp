#pragma once

#include "ActiveDevices.hpp"
#include "DeviceIds.hpp"
#include "HomeAssistDevices/HomeAssistDeviceFactory.hpp"
#include "IDevice.hpp"

#include <memory>
#include <string>

class DeviceFactory {
public:
  DeviceFactory() = default;

  void InitHomeAssistFactory(HomeAssist::WebSocket::Api &aHaApi);

  IDevice::Ptr Create(DeviceId aId);
  IDevice::Ptr CreateHomeAssistDevice(const std::string &aEntityString);

  ActiveDevices &getActiveDevices() { return mActiveDevices; }

private:
  ActiveDevices mActiveDevices;
  std::unique_ptr<HomeAssist::HomeAssistDeviceFactory> mHomeAssistFactory;
};
