#include "ActiveDevices.hpp"

#include "HardwareFactory.hpp"

void ActiveDevices::addDevice(IDevice::Ptr device) {
  if (device) {
    mDevices.push_back(device);
  }
}

void ActiveDevices::removeDevice(const std::string& deviceName) {
  auto it = std::find_if(mDevices.begin(), mDevices.end(),
                         [&deviceName](const IDevice::Ptr& dev) {
                           return dev->GetName() == deviceName;
                         });

  if (it != mDevices.end() && (*it)->IsDeleteAble()) {
    mDevices.erase(it);
  }
}

bool ActiveDevices::handleKeyEvent(KeyPressAbstract::KeyEvent event) {
  // Iterate through devices in reverse order (last added gets priority)
  for (auto it = mDevices.rbegin(); it != mDevices.rend(); ++it) {
    if ((*it)->HandleKeyEvent(event)) {
      return true;
    }
  }
  return false;
}

std::vector<IDevice::Ptr> ActiveDevices::getDevices() const { return mDevices; }
