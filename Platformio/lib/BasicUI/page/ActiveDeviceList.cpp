#include "ActiveDeviceList.hpp"

namespace UI::Page {

ActiveDeviceList::ActiveDeviceList(ActiveDevices &devices)
    : Base(ID::Pages::ActiveDeviceList),
      mDevices(devices),
      mDeviceList(nullptr) {
  RefreshDeviceList();
  mUpdateHandler.SetNotification(devices.getListUpdateNotification());
  mUpdateHandler = [this](auto event) { RefreshDeviceList(); };
}

void ActiveDeviceList::RefreshDeviceList() {
  auto devices = mDevices.getDevices();
  if (mDeviceList) {
    RemoveElement(mDeviceList);
  }
  mDeviceList = AddNewElement<UI::Widget::List>();

  for (const auto &device : devices) {
    mDeviceList->AddItem(
        device->GetName(),
        nullptr, // no symbol
        [] {}    // empty callback since we're just displaying devices
    );
  }
}

} // namespace UI::Page
