#pragma once
#include "DropDown.hpp"
#include "PageBase.hpp"

namespace UI::Widget {
class Label;
class Switch;
} // namespace UI::Widget

namespace UI::Page {

class SystemSettings : public Base {
public:
  SystemSettings();
  ~SystemSettings();

protected:
  std::string GetTitle() override { return "Sleep Settings"; }

private:
  Widget::Label *mTimeoutLabel;
  Widget::Label *mImuLabel;
  Widget::Switch *mImuSwitch;
  Widget::DropDown<int> *mScreenTimeOutDropDown;
  // LS = Light Sleep ******
  Widget::Label *mLSTimeoutLabel;
  Widget::Label *mLSLabel;
  Widget::Switch *mLSSwitch;
  Widget::DropDown<int> *mLSTimeOutDropDown;
  bool mSaveReqrd = false;
};

} // namespace UI::Page