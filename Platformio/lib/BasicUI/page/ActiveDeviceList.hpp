#pragma once
#include "ActiveDevices.hpp"
#include "Notification.hpp"
#include "PageBase.hpp"
#include "widget/List.hpp"

namespace UI::Page {

class ActiveDeviceList : public Base {
 public:
  explicit ActiveDeviceList(ActiveDevices& devices);
  std::string GetTitle() override { return "Active Devices"; }

 private:
  void RefreshDeviceList();

  ActiveDevices& mDevices;
  UI::Widget::List* mDeviceList;
  Handler<ActiveDevices::ListEvent> mUpdateHandler;
};

}  // namespace UI::Page
