#pragma once
#include <string>

#include "DeviceFactory.hpp"
#include "HardwareAbstract.hpp"
#include "HomeScreen.hpp"
#include "List.hpp"
#include "MainTopBar.hpp"
#include "PageBase.hpp"
#include "ScreenBase.hpp"
#include "StatusBar.hpp"

namespace UI::Screen {

class JsonHomeScreen : public HomeScreen {
public:
  JsonHomeScreen(DeviceFactory &factory);

protected:
  void displayScenePage(std::string aFileName);

private:
  Widget::List *mList;
};

} // namespace UI::Screen