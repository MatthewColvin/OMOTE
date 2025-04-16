#include "SettingsPage.hpp"
#include "BackgroundScreen.hpp"
#include "Button.hpp"
#include "Colors.hpp"
#include "DisplaySettings.hpp"
#include "HardwareFactory.hpp"
#include "IrLearner.hpp"
#include "List.hpp"
#include "LoggingSettings.hpp"
#include "MqttSettings.hpp"
#include "PopUpScreen.hpp"
#include "ScreenManager.hpp"
#include "Slider.hpp"
#include "SystemSettings.hpp"
#include "WifiSettings.hpp"

using namespace UI::Page;
using namespace UI::Color;

SettingsPage::SettingsPage()
    : Base(ID::Pages::Settings), mSettingsList(AddNewElement<Widget::List>()) {

  mSettingsList->AddItem("Display", LV_SYMBOL_EYE_OPEN,
                         [this] { PushDisplaySettings(); });
  mSettingsList->AddItem("Wifi", LV_SYMBOL_WIFI,
                         [this] { PushWifiSettings(); });
  mSettingsList->AddItem("MQTT", LV_SYMBOL_HOME,
                         [this] { PushMqttSettings(); });
  mSettingsList->AddItem("System", LV_SYMBOL_SETTINGS,
                         [this] { PushSystemSettings(); });
  mSettingsList->AddItem("IR Receiver", LV_SYMBOL_SETTINGS,
                         [this] { PushIrReader(); });
  mSettingsList->AddItem("Logging", LV_SYMBOL_LIST,
                         [this] { PushLoggingSettings(); });
}

void SettingsPage::AddSettingItem(std::string aTitle, const char *aSymbol,
                                  std::function<Base::Ptr()> aPageGetter) {
  mSettingsList->AddItem(aTitle, aSymbol, [aPageGetter] {
    if (auto page = aPageGetter(); page) {
      UI::Screen::Manager::getInstance().pushPopUp(std::move(page));
    }
  });
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
