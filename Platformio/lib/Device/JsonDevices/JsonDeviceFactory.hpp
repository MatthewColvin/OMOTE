#pragma once

#include "IDevice.hpp"
#include "RapidJsonUtilty.hpp"
#include "JsonDevices/KeyAction.hpp"
#include <memory>
#include <string>

namespace Json {

class JsonDeviceFactory {
public:
  JsonDeviceFactory();
  IDevice::Ptr Create(const MemConciousValue &aDeviceJson);
};

} // namespace Json
