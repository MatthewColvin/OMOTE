#pragma once

#include "ActiveDevices.hpp"
#include "Button.hpp"
#include "Label.hpp"
#include "WidgetBase.hpp"

namespace UI::Widget {

class StatusBar : public Base {
public:
  static constexpr auto Height = 0.0625f * SCREEN_HEIGHT;
  static constexpr lv_opa_t Transparency = LV_OPA_20;

  StatusBar(ActiveDevices &aActiveDevices);

private:
  void SettingsPress();
  void ActiveListPress();

  ActiveDevices &mActiveDevices;

  Widget::Button *mTopBarSettingsButton;
  Widget::Button *mTopBarActiveListButton;
  Widget::Label *mTopBarWiFiLabel;
  Widget::Label *mTopBarSOCLabel;
  Widget::Label *mTopBarBatteryLabel;
  Widget::Label *mTopBarActiveListLabel;
};

} // namespace UI::Widget
