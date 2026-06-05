#include "SettingsPage.hpp"
#include "BackgroundScreen.hpp"
#include "Button.hpp"
#include "Colors.hpp"
#include "DisplaySettings.hpp"
#include "HardwareFactory.hpp"
#include "IrLearner.hpp"
#include "LearnBattery.hpp"
#include "List.hpp"
#include "LoggingSettings.hpp"
#include "PopUpScreen.hpp"
#include "ScreenManager.hpp"
#include "Slider.hpp"
#include "SystemSettings.hpp"
#include "WifiSettings.hpp"
#include "device_settings_schema.hpp"

#include <lvgl.h>

using namespace UI::Page;
using namespace UI::Color;

static constexpr auto SettingItemHeight = 45;

namespace {

/** Map schema menu_icon / section id to LVGL symbol (Settings list). */
const char *menuIconForSection(const rapidjson::Value &section) {
  auto mapIcon = [](const char *name) -> const char * {
    if (!name)
      return LV_SYMBOL_SETTINGS;
    if (strcmp(name, "wifi") == 0)
      return LV_SYMBOL_WIFI;
    if (strcmp(name, "home") == 0)
      return LV_SYMBOL_HOME;
    if (strcmp(name, "refresh") == 0)
      return LV_SYMBOL_REFRESH;
    if (strcmp(name, "directory") == 0)
      return LV_SYMBOL_DIRECTORY;
    if (strcmp(name, "list") == 0)
      return LV_SYMBOL_LIST;
    if (strcmp(name, "battery") == 0 || strcmp(name, "battery_3") == 0)
      return LV_SYMBOL_BATTERY_3;
    if (strcmp(name, "eye_open") == 0)
      return LV_SYMBOL_EYE_OPEN;
    if (strcmp(name, "settings") == 0)
      return LV_SYMBOL_SETTINGS;
    if (strcmp(name, "charge") == 0)
      return LV_SYMBOL_CHARGE;
    if (strcmp(name, "usb") == 0)
      return LV_SYMBOL_USB;
    if (strcmp(name, "close") == 0)
      return LV_SYMBOL_CLOSE;
    return LV_SYMBOL_SETTINGS;
  };

  if (section.HasMember("menu_icon") && section["menu_icon"].IsString())
    return mapIcon(section["menu_icon"].GetString());

  if (section.HasMember("id") && section["id"].IsString()) {
    const char *id = section["id"].GetString();
    if (strcmp(id, "mqtt") == 0)
      return LV_SYMBOL_HOME;
    if (strcmp(id, "ntp") == 0)
      return LV_SYMBOL_REFRESH;
    if (strcmp(id, "ftp") == 0)
      return LV_SYMBOL_DIRECTORY;
  }

  return LV_SYMBOL_SETTINGS;
}

} // namespace

SettingsPage::SettingsPage()
    : Base(ID::Pages::Settings), mSettingsList(AddNewElement<Widget::List>()) {

  mSettingsList->AddItem("Backlight", LV_SYMBOL_SETTINGS, [this] { PushDisplaySettings(); }, SettingItemHeight);
  mSettingsList->AddItem("Device", LV_SYMBOL_SETTINGS, [this] { PushSystemSettings(); }, mHeight);

  device_settings_schema::loadFromLittleFS();
  const auto &schema = device_settings_schema::document();
  if (schema.IsObject() && schema.HasMember("sections") && schema["sections"].IsArray()) {
    const auto &sections = schema["sections"];
    for (rapidjson::SizeType i = 0; i < sections.Size(); ++i) {
      const auto &section = sections[i];
      if (!section.IsObject() || !section.HasMember("id") || !section["id"].IsString())
        continue;
      const char *placement =
          section.HasMember("placement") && section["placement"].IsString()
              ? section["placement"].GetString()
              : "submenu";
      if (strcmp(placement, "menu") != 0)
        continue;
      const std::string sectionId = section["id"].GetString();
      const std::string sectionTitle =
          (section.HasMember("menu_title") && section["menu_title"].IsString())
              ? section["menu_title"].GetString()
              : (section.HasMember("title") && section["title"].IsString() ? section["title"].GetString()
                                                                             : sectionId);
      mSettingsList->AddItem(sectionTitle, menuIconForSection(section), [sectionId, sectionTitle] {
        UI::Screen::Manager::getInstance().pushPopUp(
            std::make_unique<SystemSettings>(sectionId, sectionTitle));
      }, SettingItemHeight);
    }
  }

  mSettingsList->AddItem("Wifi", LV_SYMBOL_WIFI, [this] { PushWifiSettings(); }, SettingItemHeight);
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
