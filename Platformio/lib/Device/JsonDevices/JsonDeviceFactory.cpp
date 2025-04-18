#include "JsonDevices/JsonDeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "JsonDevices/JsonDevice.hpp"

namespace Json {

JsonDeviceFactory::JsonDeviceFactory() {}

IDevice::Ptr JsonDeviceFactory::Create(const MemConciousValue &aDeviceJson) {
  return std::make_shared<JsonDevice>(aDeviceJson);
}

IDevice::Ptr JsonDeviceFactory::Create(const std::string &aFilePathToDevice) {
  auto deviceFile = HardwareFactory::getAbstract().littleFs()->open(aFilePathToDevice);
  constexpr auto maxDeviceFileSize = 2000;
  if (deviceFile.size() > maxDeviceFileSize) {
    return nullptr;
  }

  auto deviceJsonStr = deviceFile.read(maxDeviceFileSize);
  MemConsciousDocument deviceJson;
  deviceJson.Parse(deviceJsonStr.c_str());

  return Create(deviceJson);
}

} // namespace Json
