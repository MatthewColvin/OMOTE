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

  mTabView->SetHeight(ContentHeight);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);

  // Adds pages to the Tab view
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

std::unique_ptr<UI::Page::TabView> HomeScreen::SwapTabView(std::unique_ptr<UI::Page::TabView> aNewTabView) {
  auto oldTabView = RemoveElement(mTabView);
  mTabView = AddElement(std::move(aNewTabView));
  mTabView->SetHeight(ContentHeight);
  mTabView->AlignTo(mStatusBar, LV_ALIGN_OUT_BOTTOM_MID);
  return std::unique_ptr<UI::Page::TabView>(static_cast<UI::Page::TabView *>(oldTabView.release()));
}
