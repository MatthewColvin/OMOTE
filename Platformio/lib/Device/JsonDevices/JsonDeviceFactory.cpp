#include "JsonDevices/JsonDeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "JsonDevices/JsonDevice.hpp"

namespace Json {

JsonDeviceFactory::JsonDeviceFactory() : mFs(HardwareFactory::getAbstract().littleFs()) {}

IDevice::Ptr JsonDeviceFactory::Create(const std::string &aFilePathToDevice) {
  auto deviceFile = mFs->open(aFilePathToDevice);
  return Create(deviceFile);
}

IDevice::Ptr JsonDeviceFactory::Create(File &aDeviceFile) {
  auto device = std::make_shared<JsonDevice>(aDeviceFile);
  return device->isValid() ? device : nullptr;
}

std::vector<std::shared_ptr<IDevice>> JsonDeviceFactory::getDevices(const std::string &aDirectory) {
  if (!mFs->isDir(aDirectory)) {
    return {};
  }

  std::vector<std::shared_ptr<IDevice>> devices;
  for (auto &file : mFs->FilesIn(aDirectory)) {
    auto device = Create(file);
    if (device) {
      devices.push_back(device);
    }
  }

  return devices;
}

} // namespace Json
