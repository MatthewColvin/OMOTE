#include "StatusBar.hpp"

#include "ActiveDeviceList.hpp"
#include "Colors.hpp"
#include "HardwareFactory.hpp"
#include "PopUpScreen.hpp"
#include "ScreenBase.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"
#include "observerHandles.hpp"

#define BUF_SIZE 10
#define RED_SOC_THRESH 10

namespace UI::Widget {

StatusBar::StatusBar(DeviceFactory &aFactory)
    : Base(ID::Widgets::StatusBar),
      mFactory(aFactory),
      mSceneChange(std::make_shared<Notification<std::string>>()),
      mTopBarBatteryLabel(AddNewElement<Widget::Label>("")),
      mTopBarWiFiLabel(AddNewElement<Widget::Label>("")),
      mTopBarGeneralLabel(AddNewElement<Widget::Label>("")),
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
  mTopBarBatteryLabel->SetWidth(24);
  mTopBarBatteryLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  mTopBarWiFiLabel->SetHeight(Height);
  mTopBarWiFiLabel->SetWidth(24);
  mTopBarWiFiLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  mTopBarGeneralLabel->SetHeight(Height);
  mTopBarGeneralLabel->SetWidth(42);
  mTopBarGeneralLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));
  mTopBarActiveListLabel->SetLongMode(LV_LABEL_LONG_SCROLL_CIRCULAR);
  mTopBarActiveListLabel->SetHeight(Height);
  mTopBarActiveListLabel->SetWidth(mTopBarActiveListButton->GetWidth());
  mTopBarActiveListLabel->SetTextStyle(UI::TextStyle().Align(LV_TEXT_ALIGN_CENTER));

  mTopBarSettingsButton->AlignTo(this, LV_ALIGN_TOP_RIGHT);
  mTopBarActiveListButton->AlignTo(mTopBarSettingsButton, LV_ALIGN_OUT_LEFT_MID);
  mTopBarGeneralLabel->AlignTo(this, LV_ALIGN_TOP_RIGHT);
  mTopBarBatteryLabel->AlignTo(mTopBarGeneralLabel, LV_ALIGN_OUT_LEFT_MID);
  mTopBarWiFiLabel->AlignTo(mTopBarBatteryLabel, LV_ALIGN_OUT_LEFT_MID);
  mTopBarActiveListLabel->AlignTo(mTopBarActiveListButton, LV_ALIGN_CENTER);

  mTopBarGeneralLabel->BindTextEvent(GENERAL_STATUS, NULL);

  mTimer = lv_timer_create(StatusBar::onTimer, 100, this);
}

StatusBar::~StatusBar() {
  if (mTimer) {
    lv_timer_del(mTimer);
    mTimer = nullptr;
  }
}

void StatusBar::onTimer(_lv_timer_t *aTimer) {
  StatusBar *statusBar = reinterpret_cast<StatusBar *>(lv_timer_get_user_data(aTimer));

  int32_t iSoc = HardwareFactory::getAbstract().battery()->getPercentage();
  if (HardwareFactory::getAbstract().battery()->isCharging())
    statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_CHARGE);
  else if (HardwareFactory::getAbstract().isUsbConnected())
    statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_USB);
  else {
    if (iSoc < RED_SOC_THRESH) {
      statusBar->mTopBarBatteryLabel->SetTextStyle(UI::TextStyle().Color(UI::Color::RED));
    } else {
      statusBar->mTopBarBatteryLabel->SetTextStyle(UI::TextStyle().Color(UI::Color::WHITE));
      if (iSoc < 13)
        statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_BATTERY_EMPTY);
      else if (iSoc < 38)
        statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_BATTERY_1);
      else if (iSoc < 63)
        statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_BATTERY_2);
      else if (iSoc < 88)
        statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_BATTERY_3);
      else
        statusBar->mTopBarBatteryLabel->SetText(LV_SYMBOL_BATTERY_FULL);
    }
  }

  char strftime_buf[BUF_SIZE];
  bool displayTime = false;
  struct tm timeinfo;
  if (HardwareFactory::getAbstract().wifi()->isNtpEnabled()) {
    uint32_t secSinceWake = (HardwareFactory::getAbstract().getMillis() - HardwareFactory::getAbstract().getWakeTime()) / 1000;
    time_t tNow = time(NULL);
#ifdef _WIN32
    localtime_s(&timeinfo, &tNow);
#else
    localtime_r(&tNow, &timeinfo);
#endif

    switch (HardwareFactory::getAbstract().wifi()->ntpGetDisplayMode()) {
    case ntpDisplayMode::constant:
    default:
      displayTime = true;
      break;
    case ntpDisplayMode::alternates:
      if ((secSinceWake % 4) < 2)
        displayTime = true;
      break;
    case ntpDisplayMode::first5sec:
      if (secSinceWake < 5)
        displayTime = true;
      break;
    }
  }

  if (displayTime) {
    statusBar->mTopBarGeneralLabel->SetTextStyle(UI::TextStyle().Color(UI::Color::WHITE));
    if (timeinfo.tm_year < (2016 - 1900))
      snprintf(strftime_buf, BUF_SIZE, "");
    else
      strftime(strftime_buf, sizeof(strftime_buf), "%H:%M", &timeinfo);
  } else {
    snprintf(strftime_buf, BUF_SIZE, "%i%%", iSoc);
    if (iSoc < RED_SOC_THRESH)
      statusBar->mTopBarGeneralLabel->SetTextStyle(UI::TextStyle().Color(UI::Color::RED));
    else
      statusBar->mTopBarGeneralLabel->SetTextStyle(UI::TextStyle().Color(UI::Color::WHITE));
  }

  statusBar->mTopBarGeneralLabel->SetText(strftime_buf);

  wifiHandlerInterface::wifiStatus wifiStatus = HardwareFactory::getAbstract().wifi()->GetStatus();
  if (wifiStatus.isConnected)
    statusBar->mTopBarWiFiLabel->SetText(LV_SYMBOL_WIFI);
  else
    statusBar->mTopBarWiFiLabel->SetText("");
}

void StatusBar::AddExtraSettingItem(UI::Page::SettingsPage::InjectedItem aItem) {
  mExtraSettingsItems.push_back(aItem);
}

void StatusBar::SetTopButtonLabel(std::string aLabel) {
  mTopBarActiveListLabel->SetText(aLabel);
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
  // UI::Screen::Manager::getInstance().pushPopUp(
  //     std::make_unique<Page::ActiveDeviceList>(mFactory), LV_SCR_LOAD_ANIM_OVER_BOTTOM);
  //  Example of triggering handler to run using notify
  mSceneChange->notify("TheNewScene");
}

} // namespace UI::Widget
