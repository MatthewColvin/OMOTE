#pragma once
#include "BasicUI.hpp"
#include "DeviceFactory.hpp"
#include "PageBase.hpp"

namespace UI {

namespace Screen {
class JsonHomeScreen;
}

class JsonUI : public BasicUI {
public:
  JsonUI();

  void InitHomeScreen() override;
  void AddPageToHomeScreen(Page::Base::Ptr aPageToAdd) override;
  bool GoToPage(ID anId) override;
  void loopHandler() override;

private:
  Screen::JsonHomeScreen *mJsonHomeScreen;

}; // namespace UIBase

} // namespace UI