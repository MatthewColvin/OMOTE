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
  std::string GetTitle() override { return "System Settings"; }

private:
  Widget::Label *mTimeoutLabel;
  Widget::Label *mImuLabel;
  Widget::Switch *mImuSwitch;
  Widget::DropDown<int> *mScreenTimeOutDropDown;
};

} // namespace UI::Page