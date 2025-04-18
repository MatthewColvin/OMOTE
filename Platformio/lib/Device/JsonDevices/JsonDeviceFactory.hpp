#pragma once

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
};

} // namespace Json
