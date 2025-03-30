#include "ActiveDevices.hpp"

#include "HardwareFactory.hpp"
void ActiveDevices::addDevice(IDevice::Ptr device) {
  // Add new devices to the front of the list since that will be highest
  // priority
  if (device) {
    mDevices.push_front(device);
    mListUpdated->notify(ListEvent::Added);
  }
}

void ActiveDevices::removeDevice(const std::string &deviceName) {
  auto it = std::find_if(
      mDevices.begin(), mDevices.end(),
      [&deviceName](IDevice::Ptr dev) { return dev->GetName() == deviceName; });

  if (it != mDevices.end() && (*it)->IsDeleteAble()) {
    mDevices.erase(it);
    mListUpdated->notify(ListEvent::Removed);
  }
}

bool ActiveDevices::handleKeyEvent(KeyPressAbstract::KeyEvent event) {
  for (auto dev : mDevices) {
    if (dev->HandleKeyEvent(event)) {
      mDeviceHandledKeyEvent->notify(dev, event);
      return true;
    }
  }
  return false;
}

std::deque<IDevice::Ptr> ActiveDevices::getDevices() const { return mDevices; }

ActiveDevices::ListUpdatedNotification
ActiveDevices::getListUpdateNotification() {
  return mListUpdated;
}
ActiveDevices::KeyHandledNotification
ActiveDevices::getKeyPressHandledNotification() {
  return mDeviceHandledKeyEvent;
}