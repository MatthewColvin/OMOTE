#pragma once
#include "ActiveDevices.hpp"
#include "PageBase.hpp"
#include "UIBase.hpp"
namespace UI {
namespace Screen {
class HomeScreen;
}

class BasicUI : public UIBase {
 public:
  BasicUI();

 protected:
  void AddPageToHomeScreen(Page::Base::Ptr aPageToAdd);
  bool GoToPage(ID anId);

 private:
  Screen::HomeScreen* mHomeScreen;
  ActiveDevices mActiveDevices;

};  // namespace UIBase

}  // namespace UI