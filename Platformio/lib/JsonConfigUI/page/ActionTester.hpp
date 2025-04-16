#pragma once
#include "Button.hpp"
#include "IAction.hpp"
#include "Label.hpp"
#include "PageBase.hpp"
#include "Roller.hpp"
#include <memory>
#include <vector>

namespace UI::Page {

class ActionTester : public Base {
public:
  ActionTester();
  virtual ~ActionTester() = default;

  std::string GetTitle() override { return "Action Tester"; }

private:
  void OnExecuteButtonClicked();

  Widget::Label *mInstructionLabel;
  Widget::Roller<IAction *> *mActionRoller;
  Widget::Button *mExecuteButton;
  std::vector<std::unique_ptr<IAction>> mActions;
};

} // namespace UI::Page
