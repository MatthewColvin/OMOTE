#include "SystemSettings.hpp"
#include "HardwareFactory.hpp"
#include "Label.hpp"
#include "Switch.hpp"

using namespace UI::Page;

SystemSettings::SystemSettings()
    : Base(ID::Pages::SystemSettings),
      mImuLabel(AddNewElement<Widget::Label>("Wake On Movement")),
      mImuSwitch(AddNewElement<Widget::Switch>([this](auto aNewState) {
        HardwareFactory::getAbstract().setWakeupByIMUEnabled(aNewState);},
        HardwareFactory::getAbstract().getWakeupByIMUEnabled())),
      mTimeoutLabel(AddNewElement<Widget::Label>("TimeOut")), mScreenTimeOutDropDown(AddNewElement<Widget::DropDown<int>>([this](int aTimeout) {
        HardwareFactory::getAbstract().setSleepTimeout(aTimeout);
      })) {

  mImuLabel->SetSize(lv_pct(80), 15);
  mImuLabel->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 15);

  mImuSwitch->SetSize(lv_pct(20), 15);
  mImuSwitch->AlignTo(mImuLabel, LV_ALIGN_OUT_RIGHT_MID);

  mTimeoutLabel->SetHeight(15);
  mTimeoutLabel->AlignTo(mImuLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);

  mScreenTimeOutDropDown->SetHeight(30);
  mScreenTimeOutDropDown->SetWidth(GetContentWidth());
  mScreenTimeOutDropDown->AddItem("10 Seconds", 10000);
  mScreenTimeOutDropDown->AddItem("15 Seconds", 15000);
  mScreenTimeOutDropDown->AddItem("20 Seconds", 20000);
  mScreenTimeOutDropDown->AddItem("1 Min", 60000);
  mScreenTimeOutDropDown->AlignTo(mTimeoutLabel, LV_ALIGN_OUT_BOTTOM_MID);
  mScreenTimeOutDropDown->SetSelected(
      HardwareFactory::getAbstract().getSleepTimeout());
}

SystemSettings::~SystemSettings() {
  HardwareFactory::getAbstract().saveSettings();
}