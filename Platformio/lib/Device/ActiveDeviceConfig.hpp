#pragma once

#include "ActiveDevices.hpp"
#include "DeviceIds.hpp"
#include "IDevice.hpp"
#include "Notification.hpp"
#include "rapidjson/document.h"
#include <memory>
#include <vector>

class DeviceFactory;

class ActiveDeviceConfig {
public:
  ActiveDeviceConfig(DeviceFactory &factory);

  // Save current devices to config
  bool saveDevices(const std::deque<IDevice::Ptr> &devices);

  // Load and create devices from config
  std::vector<IDevice::Ptr> loadDevices();

private:
  static constexpr auto ACTIVE_DEVICES_CONFIG_FILE = "/activeDevices.json";
  DeviceFactory &mFactory;

  Handler<ActiveDevices::ListEvent> mSaveOnChangeHandler;

  std::shared_ptr<IDevice> createHomeAssistDevice(const MemConciousValue &aActiveDeviceJsonConfigMember);
  std::shared_ptr<IDevice> createJsonDevice(const MemConciousValue &aActiveDeviceJsonConfigMember);
};
