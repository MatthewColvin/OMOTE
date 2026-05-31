#pragma once
#include "PageBase.hpp"

namespace UI::Widget {
class Label;
}

namespace UI::Page {

class EditorSyncPage : public Base {
public:
  EditorSyncPage();
  bool OnKeyEvent(KeyPressAbstract::KeyEvent aKeyEvent) override;
  std::string GetTitle() override { return "Editor sync"; }

private:
  Widget::Label *mTitle;
  Widget::Label *mBody;
  Widget::Label *mIp;
};

} // namespace UI::Page
