#pragma once

#include "IDevice.hpp"
#include "JsonDevices/KeyAction.hpp"
#include "RapidJsonUtilty.hpp"

#include <filesystem>
#include <memory>
#include <string>

namespace Json {

class JsonDeviceFactory {
public:
  JsonDeviceFactory() = default;
  virtual ~JsonDeviceFactory() = default;

  IDevice::Ptr Create(const MemConciousValue &aDeviceJson);
  IDevice::Ptr Create(const std::filesystem::path &aDeviceFile);

  // Given a Directory path, loop over files that define devices and return them
  std::vector<std::shared_ptr<IDevice>> getDevices(const std::filesystem::path &aDevicesDirectory = "/Devices");
};

} // namespace Json
