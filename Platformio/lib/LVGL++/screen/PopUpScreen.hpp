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
  PopUpScreen(UI::Page::Base::Ptr aPage);

  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;

private:
  UI::Page::Base *mContentPage = nullptr;
  Widget::Button *mExitButton = nullptr;
  Widget::Label *mTitle = nullptr;
  Widget::Image *mXsymbol = nullptr;
};

} // namespace UI::Screen