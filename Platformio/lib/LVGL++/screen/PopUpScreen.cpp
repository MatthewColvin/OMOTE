#include "PopUpScreen.hpp"

#include "Button.hpp"
#include "Colors.hpp"
#include "Image.hpp"
#include "Label.hpp"
#include "ScreenManager.hpp"

using namespace UI;
using namespace UI::Screen;

PopUpScreen::PopUpScreen(Page::Base::Ptr aPage, PageLoadedCallableTy aOnLoadComplete)
    : Screen::Base(UI::ID::Screens::PopUp), mOnLoadComplete(std::move(aOnLoadComplete)) {
  mContentPage = AddElement(std::move(aPage));

  mExitButton = AddNewElement<Widget::Button>(
      [this] { UI::Screen::Manager::getInstance().popScreen(this); });

  mTitle = AddNewElement<Widget::Label>(mContentPage->GetTitle());

  mXsymbol = AddNewElement<Widget::Image>(LV_SYMBOL_CLOSE);

  mExitButton->SetWidth(lv_pct(15));
  mExitButton->SetHeight(mExitButton->GetWidth());
  mExitButton->SetBgColor(Color::RED);
  mExitButton->AlignTo(this, LV_ALIGN_TOP_RIGHT, -5, 5);

  mXsymbol->MatchContentDimentions(mExitButton);
  mXsymbol->SetZoom(1024);
  mXsymbol->AlignTo(mExitButton, LV_ALIGN_CENTER);

  mTitle->SetWidth(mExitButton->GetX());
  mTitle->SetHeight(mExitButton->GetContentHeight());
  mTitle->AlignTo(mExitButton, LV_ALIGN_OUT_LEFT_MID);
  mTitle->SetTextStyle(mTitle->GetTextStyle()
                           .Align(LV_TEXT_ALIGN_CENTER)
                           .Font(&lv_font_montserrat_16));

  mContentPage->SetHeight(GetHeight() - mExitButton->GetBottom() - 5);
  mContentPage->SetY(mExitButton->GetBottom() + 5);
}

bool PopUpScreen::OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) {
  return mContentPage->OnKeyEvent(aKeyEvent);
}

void PopUpScreen::OnLvglEvent(lv_event_t *aEvent) {
  if (lv_event_get_code(aEvent) == LV_EVENT_SCREEN_LOADED) {
    mTimesLoaded++;
    if (mOnLoadComplete) {
      mOnLoadComplete(mTimesLoaded);
    }
  }
}
