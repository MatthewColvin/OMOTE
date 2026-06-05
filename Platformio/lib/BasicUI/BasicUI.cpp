#include "BasicUI.hpp"

#include "Hardware/KeyPressAbstract.hpp"
#include "HardwareFactory.hpp"
#include "HomeScreen.hpp"
#include "ScreenManager.hpp"
#include "editor_sync_mode.hpp"

using namespace UI;

BasicUI::BasicUI() : UIBase() {
  HardwareFactory::getAbstract().keys()->RegisterKeyPressHandler(
      [this](auto aKeyEvent) {
        if (editor_sync_mode::isActive() &&
            aKeyEvent.mId == KeyPressAbstract::KeyId::Power &&
            aKeyEvent.mType == KeyPressAbstract::KeyEvent::Type::Press) {
          editor_sync_mode::exit(true);
          return true;
        }
        if (Screen::Manager::getInstance().distributeKeyEvent(aKeyEvent)) {
          return true;
          // Pass key event to devices to handle if not
        } else if (mDeviceFactory.getActiveDevices().handleKeyEvent(aKeyEvent)) {
          return true;
        } else {
          // Could potentially add a check here and display that a key event was
          // unused.
          return false;
        }
      });
}

void BasicUI::InitHomeScreen() {
  auto homeScreen = std::make_unique<Screen::HomeScreen>(mDeviceFactory);
  mHomeScreen = homeScreen.get();
  Screen::Manager::getInstance().pushScreen(std::move(homeScreen));
}

void BasicUI::restore() {
  InitHomeScreen();
  mDeviceFactory.restoreFromConfig();
}

void BasicUI::AddPageToHomeScreen(std::unique_ptr<Page::Base> aPageToAdd) {
  mHomeScreen->AddPage(std::move(aPageToAdd));
}

bool BasicUI::GoToPage(ID anId) { return mHomeScreen->GoToPage(anId); }
