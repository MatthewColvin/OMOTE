#include "BasicUI.hpp"

#include "HardwareFactory.hpp"
#include "HomeScreen.hpp"
#include "ScreenManager.hpp"

using namespace UI;

BasicUI::BasicUI() : UIBase() {
  HardwareFactory::getAbstract().keys()->RegisterKeyPressHandler(
      [this](auto aKeyEvent) {
        // See if any UI elements wanted the key press first
        if (Screen::Manager::getInstance().distributeKeyEvent(aKeyEvent)) {
          return true;
          // Pass key event to devices to handle if not
        } else if (mActiveDevices.handleKeyEvent(aKeyEvent)) {
          return true;
        } else {
          // Could potentially add a check here and display that a key event was
          // unused.
          return false;
        }
      });

  auto homeScreen = std::make_unique<Screen::HomeScreen>();
  mHomeScreen = homeScreen.get();
  Screen::Manager::getInstance().pushScreen(std::move(homeScreen));

  HardwareFactory::getAbstract().wifi()->begin();
}

void BasicUI::AddPageToHomeScreen(std::unique_ptr<Page::Base> aPageToAdd) {
  mHomeScreen->AddPage(std::move(aPageToAdd));
}

bool BasicUI::GoToPage(ID anId) { return mHomeScreen->GoToPage(anId); }
