#include "JsonUI.hpp"

#include "HardwareFactory.hpp"
#include "JsonHomeScreen.hpp"
#include "ScreenManager.hpp"

using namespace UI;

JsonUI::JsonUI() : BasicUI() {
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