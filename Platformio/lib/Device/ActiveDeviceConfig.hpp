#pragma once

#include "DeviceFactory.hpp"
#include "DeviceIds.hpp"
#include "IDevice.hpp"
#include "littlefs/flashLittleFs.hpp"
#include "rapidjson/document.h"
#include <memory>
#include <vector>

class ActiveDeviceConfig {
public:
  ActiveDeviceConfig(std::shared_ptr<LittleFsInterface> fs, DeviceFactory &factory);

  // Save current devices to config
  bool saveDevices(const std::vector<IDevice::Ptr> &devices);

  // Load and create devices from config
  std::vector<IDevice::Ptr> loadDevices();

private:
  static constexpr auto ACTIVE_DEVICES_CONFIG_FILE = "/devices.json";
  std::shared_ptr<LittleFsInterface> mFs;
  DeviceFactory &mFactory;
};
