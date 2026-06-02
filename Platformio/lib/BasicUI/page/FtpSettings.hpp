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
enum ftpField { mDNS_name,
                ftp_user,
                ftp_password };

class FtpSettings : public Base {
  using WifiInfo = wifiHandlerInterface::WifiInfo;

public:
  // TODO: Evaluate if interface for just FTP can be here instead of full wifi interface
  FtpSettings(std::shared_ptr<wifiHandlerInterface> aWifi);
  ~FtpSettings();

  std::string GetTitle() override { return "FTP Settings"; };

  void SetHeight(lv_coord_t aHeight) override;

protected:
  void OpenKeyboard(ftpField aField, std::string aText);

private:
  std::shared_ptr<wifiHandlerInterface> mWifi;

  UI::Widget::List *mList;
  UI::Widget::Keyboard *mKeyboard;
  UI::Widget::Label *mEnLabel;
  UI::Widget::Switch *mEnSwitch;
  Widget::Label *mLabel;
  bool mSaveReqrd = false;
};

} // namespace UI::Page