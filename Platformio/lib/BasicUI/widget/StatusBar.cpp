#include "StatusBar.hpp"

#include "ActiveDeviceList.hpp"
#include "Colors.hpp"
#include "PopUpScreen.hpp"
#include "ScreenBase.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"
#include "observerHandles.hpp"

namespace UI::Widget {

StatusBar::StatusBar(DeviceFactory &aFactory)
    : Base(ID::Widgets::StatusBar),
      mFactory(aFactory),
      mTopBarBatteryLabel(AddNewElement<Widget::Label>("")),
      mTopBarWiFiLabel(AddNewElement<Widget::Label>("")),
      mTopBarSOCLabel(AddNewElement<Widget::Label>("--%")),
      mTopBarSettingsButton(AddNewElement<Widget::Button>([this] { SettingsPress(); })),
      mTopBarActiveListButton(AddNewElement<Widget::Button>([this] { ActiveListPress(); })),
      mTopBarActiveListLabel(AddNewElement<Widget::Label>("Active List")) {
  SetHeight(Height);
  SetBgColor(UI::Color::BLACK);

  mTopBarSettingsButton->SetHeight(Height);
  mTopBarSettingsButton->SetWidth(90);
  mTopBarSettingsButton->SetBgOpacity(Transparency);
  mTopBarActiveListButton->SetHeight(Height);
  mTopBarActiveListButton->SetWidth(150);
  mTopBarActiveListButton->SetBgOpacity(Transparency);
  mTopBarBatteryLabel->SetHeight(Height);
  mTopBarBatteryLabel->SetWidth(15);
  mTopBarBatteryLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  mTopBarWiFiLabel->SetHeight(Height);
  mTopBarWiFiLabel->SetWidth(15);
  mTopBarWiFiLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  mTopBarSOCLabel->SetHeight(Height);
  mTopBarSOCLabel->SetWidth(40);
  mTopBarSOCLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  mTopBarActiveListLabel->SetHeight(Height);
  mTopBarActiveListLabel->SetWidth(mTopBarActiveListButton->GetWidth());
  mTopBarActiveListLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));

  mTopBarSettingsButton->AlignTo(this, LV_ALIGN_TOP_RIGHT);
  mTopBarActiveListButton->AlignTo(mTopBarSettingsButton, LV_ALIGN_OUT_LEFT_MID);
  mTopBarBatteryLabel->AlignTo(mTopBarSettingsButton, LV_ALIGN_RIGHT_MID);
  mTopBarSOCLabel->AlignTo(mTopBarBatteryLabel, LV_ALIGN_OUT_LEFT_MID);
  mTopBarWiFiLabel->AlignTo(mTopBarSOCLabel, LV_ALIGN_OUT_LEFT_MID);
  mTopBarActiveListLabel->AlignTo(mTopBarActiveListButton, LV_ALIGN_CENTER);

  mTopBarBatteryLabel->BindTextEvent(BATT_STATUS, NULL);
  mTopBarWiFiLabel->BindTextEvent(WIFI_STATUS, NULL);
  mTopBarSOCLabel->BindTextEvent(SOC_STATUS, "%d%%");
}

void StatusBar::AddExtraSettingItem(UI::Page::SettingsPage::InjectedItem aItem) {
  mExtraSettingsItems.push_back(aItem);
}

void StatusBar::SettingsPress() {
  auto settings = std::make_unique<Page::SettingsPage>();
  for (auto &item : mExtraSettingsItems) {
    settings->AddSettingItem(std::get<0>(item), std::get<1>(item), std::get<2>(item));
  }

  UI::Screen::Manager::getInstance().pushPopUp(
      std::move(settings), LV_SCR_LOAD_ANIM_OVER_BOTTOM);
}

void StatusBar::ActiveListPress() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<Page::ActiveDeviceList>(mFactory), LV_SCR_LOAD_ANIM_OVER_BOTTOM);
}

} // namespace UI::Widget
