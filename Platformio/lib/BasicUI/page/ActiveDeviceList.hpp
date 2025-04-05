#pragma once
#include "DeviceFactory.hpp"
#include "Notification.hpp"
#include "PageBase.hpp"
#include "widget/List.hpp"

namespace UI::Page {

class ActiveDeviceList : public Base {
public:
  explicit ActiveDeviceList(DeviceFactory &factory);
  std::string GetTitle() override { return "Active Devices"; }

private:
  void RefreshDeviceList();

  DeviceFactory &mFactory;
  UI::Widget::List *mDeviceList;
  Handler<ActiveDevices::ListEvent> mUpdateHandler;
};

} // namespace UI::Page
