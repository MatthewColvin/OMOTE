#pragma once

#include "ActiveDevices.hpp"
#include "DeviceIds.hpp"
#include "IDevice.hpp"
#include "Notification.hpp"
#include "ObjectSchemaBuilder.hpp"
#include "ValidationFactory.hpp"
#include "rapidjson/document.h"
#include <memory>
#include <vector>

class DeviceFactory;

class ActiveDeviceConfig {
private:
  static constexpr inline auto DeviceSchema = OMOTE::JSON::ObjectSchema()
                                                  .Require("type", "integer")
                                                  .Require("id", "integer")
                                                  .Require("config", "object")
                                                  .Build();

public:
  using ConfigJsonValidator = OMOTE::JSON::ValidationFactory<DeviceId, DeviceSchema>;
  ActiveDeviceConfig(DeviceFactory &factory);

  // Save current devices to config
  bool saveDevices(const std::deque<IDevice::Ptr> &devices);

  // Load and create devices from config
  std::vector<IDevice::Ptr> loadDevices();

private:
  static constexpr auto ACTIVE_DEVICES_CONFIG_FILE = FS_PATH "activeDevices.json";
  DeviceFactory &mFactory;
  // Validator for all Devices and Secondarily for CompileTime
  ConfigJsonValidator mDeviceJsonValidator;

  Handler<ActiveDevices::ListEvent> mSaveOnChangeHandler;

  std::shared_ptr<IDevice> createHomeAssistDevice(const rapidjson::Value &aActiveDeviceJsonConfigMember);
  std::shared_ptr<IDevice> createJsonDevice(const rapidjson::Value &aActiveDeviceJsonConfigMember);
};
