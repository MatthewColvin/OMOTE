#pragma once

#include "ActiveDeviceConfig.hpp"
#include "ActiveDevices.hpp"
#include "DeviceIds.hpp"
#include "HomeAssistDevices/HomeAssistDeviceFactory.hpp"
#include "IDevice.hpp"

#include <memory>
#include <string>

class ActiveDeviceConfig;

class DeviceFactory {
public:
  DeviceFactory();

  void InitHomeAssistFactory(HomeAssist::WebSocket::Api &aHaApi);

  IDevice::Ptr Create(DeviceId aId);
  IDevice::Ptr CreateHomeAssistDevice(const std::string &aEntityString);

  ActiveDevices &getActiveDevices() { return mActiveDevices; }

  void restoreFromConfig();

private:
  // Current Devices Active in system
  ActiveDevices mActiveDevices;
  // Config that managed saving and restoring
  std::unique_ptr<ActiveDeviceConfig> mDeviceConfig = nullptr;

  // Optionally loaded Extra Device Factories
  std::unique_ptr<HomeAssist::HomeAssistDeviceFactory> mHomeAssistFactory = nullptr;
};
