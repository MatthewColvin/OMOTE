#include "ActiveDeviceList.hpp"

namespace UI::Page {

ActiveDeviceList::ActiveDeviceList(DeviceFactory &aFactory)
    : Base(ID::Pages::ActiveDeviceList),
      mFactory(aFactory),
      mDeviceList(nullptr) {
  RefreshDeviceList();
  mUpdateHandler.SetNotification(mFactory.getActiveDevices().getListUpdateNotification());
  mUpdateHandler = [this](auto event) { RefreshDeviceList(); };
}

void ActiveDeviceList::RefreshDeviceList() {
  auto devices = mFactory.getActiveDevices().getDevices();
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
