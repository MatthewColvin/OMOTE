#pragma once
#include "Button.hpp"
#include "DropDown.hpp"
#include "Hardware/wifi/wifiHandlerInterface.h"
#include "PageBase.hpp"

namespace UI::Widget {
class List;
class Label;
class Switch;
class Keyboard;
} // namespace UI::Widget

namespace UI::Page {
enum ntpField { server,
                timezone };

class NtpSettings : public Base {
  using WifiInfo = wifiHandlerInterface::WifiInfo;

public:
  // TODO: Evaluate if interface for just NTP can be here instead of full wifi interface
  NtpSettings(std::shared_ptr<wifiHandlerInterface> aWifi);
  ~NtpSettings();

  std::string GetTitle() override { return "Ntp Settings"; };

  void SetHeight(lv_coord_t aHeight) override;

protected:
  void OpenKeyboard(ntpField aField, std::string aText);

private:
  std::shared_ptr<wifiHandlerInterface> mWifi;

  UI::Widget::List *mList;
  UI::Widget::Keyboard *mKeyboard;
  UI::Widget::Label *mEnLabel;
  UI::Widget::Switch *mEnSwitch;
  Widget::Label *mDisplayLabel;
  Widget::DropDown<int> *mDisplayModeDropDown;

  bool mSaveReqrd = false;
};

} // namespace UI::Page