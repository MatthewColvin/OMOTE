#include "ActionTester.hpp"
#include "ActionFactory.hpp"

using namespace UI::Page;

ActionTester::ActionTester()
    : Base(ID::Pages::ActionTesterPage),
      mInstructionLabel(AddNewElement<Widget::Label>("Select action to test:")),
      mActionRoller(AddNewElement<Widget::Roller<IAction *>>(nullptr)),
      mExecuteButton(
          AddNewElement<Widget::Button>([this]() { OnExecuteButtonClicked(); })) {

  mInstructionLabel->SetHeight(lv_pct(10));
  mInstructionLabel->SetWidth(GetContentWidth());

  mActionRoller->SetWidth(GetContentWidth());
  mActionRoller->SetHeight(lv_pct(50));

  ActionFactory factory;
  mActions = factory.getAllActions();

  for (const auto &action : mActions) {
    mActionRoller->AddItem(action->GetName(), action.get());
  }

  mExecuteButton->SetText("Execute Action");
  mExecuteButton->SetWidth(GetContentWidth() / 2);
  mExecuteButton->SetHeight(lv_pct(20));

  mInstructionLabel->AlignTo(this, LV_ALIGN_TOP_MID);
  mActionRoller->AlignTo(mInstructionLabel, LV_ALIGN_OUT_BOTTOM_MID);
  mExecuteButton->AlignTo(mActionRoller, LV_ALIGN_OUT_BOTTOM_MID, 0, 10);
}

void ActionTester::OnExecuteButtonClicked() {
  if (IAction *selectedAction = mActionRoller->GetSelectedData()) {
    selectedAction->execute();
  }
}
