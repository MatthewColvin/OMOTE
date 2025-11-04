#include "SettingsPage.hpp"
#include "BackgroundScreen.hpp"
#include "Button.hpp"
#include "Colors.hpp"
#include "DisplaySettings.hpp"
#include "FtpSettings.hpp"
#include "HardwareFactory.hpp"
#include "IrLearner.hpp"
#include "LearnBattery.hpp"
#include "List.hpp"
#include "LoggingSettings.hpp"
#include "MqttSettings.hpp"
#include "NtpSettings.hpp"
#include "PopUpScreen.hpp"
#include "ScreenManager.hpp"
#include "Slider.hpp"
#include "SystemSettings.hpp"
#include "WifiSettings.hpp"

using namespace UI::Page;
using namespace UI::Color;

static constexpr auto SettingItemHeight = 45;

SettingsPage::SettingsPage()
    : Base(ID::Pages::Settings), mSettingsList(AddNewElement<Widget::List>()) {

  mSettingsList->AddItem("Backlight", LV_SYMBOL_SETTINGS, [this] { PushDisplaySettings(); }, SettingItemHeight);
  mSettingsList->AddItem("Sleep", LV_SYMBOL_POWER, [this] { PushSystemSettings(); }, mHeight);
  mSettingsList->AddItem("Wifi", LV_SYMBOL_WIFI, [this] { PushWifiSettings(); }, SettingItemHeight);
  mSettingsList->AddItem("MQTT", LV_SYMBOL_HOME, [this] { PushMqttSettings(); }, SettingItemHeight);
  mSettingsList->AddItem("NTP", LV_SYMBOL_REFRESH, [this] { PushNtpSettings(); }, SettingItemHeight);
  mSettingsList->AddItem("FTP", LV_SYMBOL_DIRECTORY, [this] { PushFtpSettings(); }, SettingItemHeight);
  mSettingsList->AddItem("Logging", LV_SYMBOL_LIST, [this] { PushLoggingSettings(); }, SettingItemHeight);
  mSettingsList->AddItem("Battery", LV_SYMBOL_BATTERY_3, [this] { PushLearnBattery(); }, mHeight);
  mSettingsList->AddItem("IR Receiver", LV_SYMBOL_EYE_OPEN, [this] { PushIrReader(); }, mHeight);
}

void SettingsPage::AddSettingItem(std::string aTitle, const char *aSymbol,
                                  std::function<Base::Ptr()> aPageGetter) {
  mSettingsList->AddItem(aTitle, aSymbol, [aPageGetter] {
    if (auto page = aPageGetter(); page) {
      UI::Screen::Manager::getInstance().pushPopUp(std::move(page));
    } }, SettingItemHeight);
}

void SettingsPage::PushDisplaySettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<DisplaySettings>(
          HardwareFactory::getAbstract().display()));
}

void SettingsPage::PushSystemSettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<SystemSettings>());
}

void SettingsPage::PushMqttSettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<MqttSettings>(HardwareFactory::getAbstract().wifi()));
}

void SettingsPage::PushNtpSettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<NtpSettings>(HardwareFactory::getAbstract().wifi()));
}

void SettingsPage::PushFtpSettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<FtpSettings>(HardwareFactory::getAbstract().wifi()));
}

void SettingsPage::PushWifiSettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<WifiSettings>(HardwareFactory::getAbstract().wifi()));
}

void SettingsPage::PushIrReader() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<Page::IrLearner>());
}

void SettingsPage::PushLoggingSettings() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<LoggingSettings>());
}

void SettingsPage::PushLearnBattery() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<LearnBattery>());
}
