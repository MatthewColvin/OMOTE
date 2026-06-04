#pragma once

#include "ActiveDeviceConfig.hpp"
#include "ActiveDevices.hpp"
#include "DeviceIds.hpp"
#include "HomeAssistDevices/HomeAssistDeviceFactory.hpp"
#include "IDevice.hpp"
#include "JsonDevices/JsonDeviceFactory.hpp"

#include <filesystem>
#include <memory>
#include <string>

class ActiveDeviceConfig;

class DeviceFactory {
public:
  DeviceFactory();

  void InitHomeAssistFactory(HomeAssist::WebSocket::Api &aHaApi);
  void InitJsonFactory();

  IDevice::Ptr Create(DeviceId aId);
  static bool RegisterDevice(DeviceId aId, std::function<IDevice::Ptr()> aCreator);

  IDevice::Ptr CreateHomeAssistDevice(const std::string &aEntityString);
  IDevice::Ptr CreateJsonDevice(const std::filesystem::path &aDeviceJsonFilePath);

  ActiveDevices &getActiveDevices() { return mActiveDevices; }

  // Seach the filesystem to find devices that are defined in JSON
  std::vector<std::shared_ptr<IDevice>> getJsonDevices(const std::filesystem::path &aDevicesDirectory = FS_PATH "Devices");

  // Construct a list of devices that are compiled into the firmware
  std::vector<std::shared_ptr<IDevice>> getCompileTimeDevices();

  void restoreFromConfig();

private:
  // Current Devices Active in system
  ActiveDevices mActiveDevices;
  // Config that managed saving and restoring
  std::unique_ptr<ActiveDeviceConfig> mDeviceConfig = nullptr;

  // Map used to store factory functions for custom devices defined at compile time.
  static inline std::array<std::function<IDevice::Ptr()>, static_cast<int>(DeviceId::COUNT)> sDeviceCreators;

  // Optionally loaded Extra Device Factories
  std::unique_ptr<HomeAssist::HomeAssistDeviceFactory> mHomeAssistFactory = nullptr;
  std::unique_ptr<Json::JsonDeviceFactory> mJsonFactory = nullptr;
};
