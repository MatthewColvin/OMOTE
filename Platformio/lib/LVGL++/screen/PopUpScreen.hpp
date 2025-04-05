#pragma once

#include "PageBase.hpp"
#include "ScreenBase.hpp"

namespace UI {
namespace Widget {
class Label;
class Button;
class Image;
} // namespace Widget
} // namespace UI

namespace UI::Screen {

/// @brief A Screen that allows easy display of a page that
///        can be dismissed easily by an x
class PopUpScreen : public Base {
public:
  // Function that runs on page load provides the number of times the page has loaded starting at 0
  using PageLoadedCallableTy = std::function<void(int)>;

  PopUpScreen(UI::Page::Base::Ptr aPage, PageLoadedCallableTy aOnLoadComplete = nullptr);

  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

  void OnLvglEvent(lv_event_t *aEvent);

private:
  UI::Page::Base *mContentPage = nullptr;
  PageLoadedCallableTy mOnLoadComplete = nullptr;

  uint16_t mTimesLoaded = 0;
  Widget::Button *mExitButton = nullptr;
  Widget::Label *mTitle = nullptr;
  Widget::Image *mXsymbol = nullptr;
};

} // namespace UI::Screen