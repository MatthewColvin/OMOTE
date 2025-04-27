#pragma once

#include "Button.hpp"
#include "DeviceFactory.hpp"
#include "Label.hpp"
#include "Notification.hpp"
#include "SettingsPage.hpp"
#include "WidgetBase.hpp"

#include <memory>

namespace UI::Widget {

class StatusBar : public Base {
public:
  static constexpr auto Height = 0.0625f * SCREEN_HEIGHT;
  static constexpr lv_opa_t Transparency = LV_OPA_20;

  StatusBar(DeviceFactory &factory);

  void AddExtraSettingItem(UI::Page::SettingsPage::InjectedItem aItem);
  void SetTopButtonLabel(std::string aLabel);

  Notification<std::string>::Ptr GetSceneChangeNotification() { return mSceneChange; };

private:
  void SettingsPress();
  void PushActiveDeviceList();

  DeviceFactory &mFactory;

  Notification<std::string>::Ptr mSceneChange;

  std::vector<UI::Page::SettingsPage::InjectedItem>
      mExtraSettingsItems;

  Widget::Button *mTopBarSettingsButton;
  Widget::Button *mTopBarActiveListButton;
  Widget::Label *mTopBarWiFiLabel;
  Widget::Label *mTopBarSOCLabel;
  Widget::Label *mTopBarBatteryLabel;
  Widget::Label *mTopBarActiveListLabel;
};

} // namespace UI::Widget
