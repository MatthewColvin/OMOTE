#include "HomeScreen.hpp"

#include "Colors.hpp"
#include "Demo.hpp"
#include "HardwareFactory.hpp"
#include "HeatingPage.hpp"
#include "IrLearner.hpp"
#include "PopUpScreen.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"
#include "observerHandles.hpp"

using namespace UI::Screen;

HomeScreen::HomeScreen(DeviceFactory &aFactory)
    : Base(UI::ID::Screens::Home),
      mFactory(aFactory),
      mStatusBar(AddNewElement<Widget::StatusBar>(mFactory)),
      mTabView(AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID))) {
  SetBgColor(UI::Color::BLACK);
  SetPushAnimation(LV_SCR_LOAD_ANIM_FADE_IN);

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::StatusBar::Height;
  mTabView->SetHeight(ContentHeight);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);

  // Adds pages to the Tab view
  // mTabView->AddTab(std::make_unique<Page::IrLearner>());
  mTabView->AddTab(std::make_unique<Page::Demo>());
  mTabView->AddTab(std::make_unique<Page::Heating>(HardwareFactory::getAbstract().wifi()));
}

void HomeScreen::AddPage(Page::Base::Ptr aPage) {
  mTabView->AddTab(std::move(aPage));
}

void HomeScreen::SetBgColor(lv_color_t value, lv_part_t selector) {
  mTabView->SetBgColor(value, selector);
  UI::UIElement::SetBgColor(value, selector);
}

bool HomeScreen::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
};
