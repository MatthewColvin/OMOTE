#pragma once

#include "Hardware/Littlefs/LittleFsInterface.hpp"
#include "IDevice.hpp"
#include "JsonDevices/KeyAction.hpp"
#include "RapidJsonUtilty.hpp"
#include <memory>
#include <string>

namespace Json {

class JsonDeviceFactory {
public:
  JsonDeviceFactory();
  IDevice::Ptr Create(const MemConciousValue &aDeviceJson);
  IDevice::Ptr Create(const std::string &aFilePathToDevice);
  IDevice::Ptr Create(File &aDeviceFile);
  // Given a Directory path, loop over files that define devices and return them
  std::vector<std::shared_ptr<IDevice>> getDevices(const std::string &aDirectory = "/Devices");

private:
  std::shared_ptr<LittleFsInterface> mFs = nullptr;
};

} // namespace Json
