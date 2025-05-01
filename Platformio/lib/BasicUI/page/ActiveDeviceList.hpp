#pragma once
#include "DeviceFactory.hpp"
#include "Notification.hpp"
#include "PageBase.hpp"
#include "WidgetBase.hpp"
#include "widget/SelectedItemsList.hpp"

namespace UI::Page {

class ActiveDeviceList : public Base {
public:
  explicit ActiveDeviceList(DeviceFactory &factory);
  std::string GetTitle() override { return "Active Devices"; }

private:
  void RefreshDeviceList();
  void BuildReorderControlsUI();

  void SetHeight(lv_coord_t aHeight) override;

  DeviceFactory &mFactory;
  UI::Widget::SelectedItemsList *mDeviceList = nullptr;
  std::map<UI::Widget::ListItem *, IDevice::Ptr> mDeviceListItemToDevice;
  UI::Widget::ListItem *mSelectedItem = nullptr;

  UI::Widget::Base *mReorderControls = nullptr;
  Handler<ActiveDevices::ListEvent> mUpdateHandler;
};

} // namespace UI::Page
