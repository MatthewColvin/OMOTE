#pragma once

#include <deque>

#include "Hardware/KeyPressAbstract.hpp"
#include "IDevice.hpp"
#include "Notification.hpp"

class ActiveDevices {
public:
  enum class ListEvent { Added,
                         Removed,
                         Reorder };
  using KeyHandledNotification =
      Notification<IDevice::Ptr, KeyPressAbstract::KeyEvent>::Ptr;
  using ListUpdatedNotification = Notification<ListEvent>::Ptr;

  ActiveDevices() = default;

  void restoreDevices(const std::vector<IDevice::Ptr> &aDevicesToRestore);

  void addDevice(IDevice::Ptr device);
  void removeDevice(const std::string &deviceName);
  void removeDevice(IDevice::Ptr device);
  bool handleKeyEvent(KeyPressAbstract::KeyEvent event);
  std::deque<IDevice::Ptr> getDevices() const;

  void setDevicePriority(IDevice::Ptr aDevice, int aPriority);
  int getDevicePriority(IDevice::Ptr aDevice);

  ListUpdatedNotification getListUpdateNotification();
  KeyHandledNotification getKeyPressHandledNotification();

private:
  std::deque<IDevice::Ptr> mDevices;
  // Notification fired when a KeyEvent has been handled by a specific device

  KeyHandledNotification mDeviceHandledKeyEvent = std::make_shared<
      Notification<IDevice::Ptr, KeyPressAbstract::KeyEvent>>();

  ListUpdatedNotification mListUpdated =
      std::make_shared<Notification<ListEvent>>();
};
