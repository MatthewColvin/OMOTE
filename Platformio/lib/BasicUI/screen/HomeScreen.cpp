#include "HomeScreen.hpp"

#include "Colors.hpp"
#include "Demo.hpp"
#include "IrLearner.hpp"
#include "PopUpScreen.hpp"
#include "ScreenManager.hpp"
#include "SettingsPage.hpp"
#include "observerHandles.hpp"

using namespace UI::Screen;

HomeScreen::HomeScreen(ActiveDevices& aActiveDevices)
    : Base(UI::ID::Screens::Home),
      mActiveDevices(aActiveDevices),
      mStatusBar(AddNewElement<Widget::StatusBar>()),
      mTabView(AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID))) {
  SetBgColor(UI::Color::BLACK);
  SetPushAnimation(LV_SCR_LOAD_ANIM_FADE_IN);

  static constexpr auto ContentHeight =
      SCREEN_HEIGHT - Widget::StatusBar::Height;
  mTabView->SetHeight(ContentHeight);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);

  // Adds pages to the Tab view
  mTabView->AddTab(std::make_unique<Page::IrLearner>());
  mTabView->AddTab(std::make_unique<Page::Demo>());
}

void HomeScreen::AddPage(Page::Base::Ptr aPage) {
  mTabView->AddTab(std::move(aPage));
}

void HomeScreen::SetBgColor(lv_color_t value, lv_style_selector_t selector) {
  mTabView->SetBgColor(value, selector);
  UI::UIElement::SetBgColor(value, selector);
}

bool HomeScreen::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return false;
};

void HomeScreen::SettingsPress() {
  UI::Screen::Manager::getInstance().pushPopUp(
      std::make_unique<Page::SettingsPage>(), LV_SCR_LOAD_ANIM_OVER_BOTTOM);
}

void HomeScreen::ActiveListPress() {}
