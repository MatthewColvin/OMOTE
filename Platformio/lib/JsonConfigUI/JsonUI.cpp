#include "JsonUI.hpp"

#include "EditorSyncPage.hpp"
#include "HaRuntime.hpp"
#include "HardwareFactory.hpp"
#include "JsonHomeScreen.hpp"
#include "ScreenManager.hpp"
#include "UIBase.hpp"
#include "config_reload.hpp"
#include "device_settings.hpp"
#include "editor_sync_mode.hpp"

using namespace UI;

JsonUI::JsonUI() : BasicUI() {
}

void JsonUI::loopHandler() {
  static bool syncUiShown = false;
  if (editor_sync_mode::isActive() && !syncUiShown) {
    syncUiShown = true;
    Screen::Manager::getInstance().pushPopUp(
        std::make_unique<Page::EditorSyncPage>(), LV_SCR_LOAD_ANIM_OVER_LEFT);
  }
  if (!editor_sync_mode::isActive())
    syncUiShown = false;
  if (config_reload::consumeHaSettingsDirty())
    HaRuntime::reloadSettingsFromDisk();
  if (config_reload::consumeDeviceSettingsDirty()) {
    if (device_settings::loadFromLittleFS())
      device_settings::applyToHardware();
  }
  if (mJsonHomeScreen && config_reload::consumePagesDirty())
    mJsonHomeScreen->reloadCurrentSceneFromDisk();
  HaRuntime::tick();
  UIBase::loopHandler();
}

void JsonUI::InitHomeScreen() {
  auto homeScreen = std::make_unique<Screen::JsonHomeScreen>(mDeviceFactory);
  mJsonHomeScreen = homeScreen.get();
  Screen::Manager::getInstance().pushScreen(std::move(homeScreen));
};

void JsonUI::AddPageToHomeScreen(Page::Base::Ptr aPageToAdd) {
  mJsonHomeScreen->AddPage(std::move(aPageToAdd));
};

bool JsonUI::GoToPage(ID anId) {
  return false;
};
