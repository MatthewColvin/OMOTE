#pragma once

#include <vector>

#include "IDevice.hpp"

class ActiveDevices {
 public:
  ActiveDevices() = default;

  void addDevice(IDevice::Ptr device);
  void removeDevice(const std::string& deviceName);
  bool handleKeyEvent(KeyPressAbstract::KeyEvent event);
  std::vector<IDevice::Ptr> getDevices() const;

 private:
  std::vector<IDevice::Ptr> mDevices;
};
