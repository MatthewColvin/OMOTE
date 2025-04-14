#pragma once
#include "DeviceFactory.hpp"
#include "PageBase.hpp"
#include "UIBase.hpp"

namespace UI {
namespace Screen {
class JsonHomeScreen;
}

class BasicUI : public UIBase {
public:
  BasicUI();

protected:
  void AddPageToHomeScreen(Page::Base::Ptr aPageToAdd);
  bool GoToPage(ID anId);

  void restore() override;

  DeviceFactory mDeviceFactory;

private:
  Screen::JsonHomeScreen *mHomeScreen;
}; // namespace UIBase

} // namespace UI