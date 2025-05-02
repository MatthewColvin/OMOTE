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

void ActiveDevices::restoreDevices(const std::vector<IDevice::Ptr> &aDevicesToRestore) {
  for (auto &device : aDevicesToRestore) {
    mDevices.push_back(device);
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

void ActiveDevices::removeDevice(IDevice::Ptr device) {
  auto it = std::find(mDevices.begin(), mDevices.end(), device);
  if (it != mDevices.end()) {
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

bool ActiveDevices::setDevicePriority(IDevice::Ptr aDevice, int aPriority) {
  if (aPriority < 0 || aPriority >= mDevices.size()) {
    return false;
  }
  auto it = std::find(mDevices.begin(), mDevices.end(), aDevice);
  if (it != mDevices.end()) {
    mDevices.erase(it);
    auto newPos = mDevices.begin() + aPriority;
    mDevices.insert(newPos, aDevice);
    mListUpdated->notify(ListEvent::Reorder);
    return true;
  }
  return false;
}

int ActiveDevices::getDevicePriority(IDevice::Ptr aDevice) {
  auto it = std::find(mDevices.begin(), mDevices.end(), aDevice);
  if (it != mDevices.end()) {
    return std::distance(mDevices.begin(), it);
  }
  return -1;
}

ActiveDevices::ListUpdatedNotification ActiveDevices::getListUpdateNotification() {
  return mListUpdated;
}

ActiveDevices::KeyHandledNotification ActiveDevices::getKeyPressHandledNotification() {
  return mDeviceHandledKeyEvent;
}