#pragma once
#include <string>

#include "HardwareAbstract.hpp"
#include "Label.hpp"
#include "MainTopBar.hpp"
#include "PageBase.hpp"
#include "ScreenBase.hpp"
#include "TabView.hpp"

namespace UI::Screen {

class HomeScreen : public Base {
 public:
  HomeScreen(ActiveDevices& aActiveDevices);

  void SetBgColor(lv_color_t value,
                  lv_style_selector_t selector = LV_PART_MAIN) override;

  void AddPage(Page::Base::Ptr aPage);

  bool GoToPage(ID anId) { return mTabView->GoToTab(anId); }

 protected:
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

 private:
  ActiveDevices& mActiveDevices;

  Page::TabView* mTabView;
  Widget::MainTopBar* mTopBar;
};

}  // namespace UI::Screen