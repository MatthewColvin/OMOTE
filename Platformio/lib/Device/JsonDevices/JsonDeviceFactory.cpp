#include "JsonDevices/JsonDeviceFactory.hpp"
#include "HardwareFactory.hpp"
#include "JsonDevices/JsonDevice.hpp"

namespace Json {

IDevice::Ptr JsonDeviceFactory::Create(const std::filesystem::path &aDeviceFile) {
  auto device = std::make_shared<JsonDevice>(aDeviceFile);
  return device->isValid() ? device : nullptr;
}

std::vector<std::shared_ptr<IDevice>> JsonDeviceFactory::getDevices(const std::filesystem::path &aDevicesDirectory) {
  if (!std::filesystem::is_directory(aDevicesDirectory)) {
    return {};
  }

  std::vector<std::shared_ptr<IDevice>> devices;
  for (auto &entry : std::filesystem::directory_iterator(aDevicesDirectory)) {
    if (auto device = Create(entry.path()); device) {
      devices.push_back(device);
    }
  }
  return devices;
}

} // namespace Json
