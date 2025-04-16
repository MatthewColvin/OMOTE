#include "JsonDevices/JsonDeviceFactory.hpp"
#include "JsonDevices/JsonDevice.hpp"

namespace Json {

JsonDeviceFactory::JsonDeviceFactory() {}

IDevice::Ptr JsonDeviceFactory::Create(const MemConciousValue &aDeviceJson) {
  return std::make_shared<JsonDevice>(aDeviceJson);
}

} // namespace Json
