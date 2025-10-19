#include "SystemSettings.hpp"
#include "HardwareFactory.hpp"
#include "Label.hpp"
#include "Switch.hpp"

using namespace UI::Page;

SystemSettings::SystemSettings()
    : Base(ID::Pages::SystemSettings),
      mImuLabel(AddNewElement<Widget::Label>("Wake On Movement")),
      mImuSwitch(AddNewElement<Widget::Switch>([this](auto aNewState) {
        HardwareFactory::getAbstract().setWakeupByIMUEnabled(aNewState); mSaveReqrd = true; },
                                               HardwareFactory::getAbstract().getWakeupByIMUEnabled())),
      mTimeoutLabel(AddNewElement<Widget::Label>("TimeOut")),
      mScreenTimeOutDropDown(AddNewElement<Widget::DropDown<int>>([this](int aTimeout) {
        HardwareFactory::getAbstract().setSleepTimeout(aTimeout);  mSaveReqrd = true; })),
      mLSLabel(AddNewElement<Widget::Label>("Use Light Sleep")),
      mLSSwitch(AddNewElement<Widget::Switch>([this](auto aNewState) {
        HardwareFactory::getAbstract().setLightSleepEnabled(aNewState); mSaveReqrd = true; },
                                              HardwareFactory::getAbstract().getLightSleepEnabled())),
      mLSTimeoutLabel(AddNewElement<Widget::Label>("Light Sleep Duration")),
      mLSTimeOutDropDown(AddNewElement<Widget::DropDown<int>>([this](int aTimeout) {
        HardwareFactory::getAbstract().setLightSleepTimeout(aTimeout); mSaveReqrd = true; })) {

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
  mScreenTimeOutDropDown->AddItem("10 Min", 600000);
  mScreenTimeOutDropDown->AddItem("30 Min", 1800000);
  mScreenTimeOutDropDown->AddItem("1 Hour", 3600000);
  mScreenTimeOutDropDown->AlignTo(mTimeoutLabel, LV_ALIGN_OUT_BOTTOM_MID);
  mScreenTimeOutDropDown->SetSelected(
      HardwareFactory::getAbstract().getSleepTimeout());

  mLSLabel->SetSize(lv_pct(80), 15);
  mLSLabel->AlignTo(mTimeoutLabel, LV_ALIGN_BOTTOM_LEFT, 0, 80);

  mLSSwitch->SetSize(lv_pct(20), 15);
  mLSSwitch->AlignTo(mLSLabel, LV_ALIGN_OUT_RIGHT_MID);

  mLSTimeoutLabel->SetHeight(15);
  mLSTimeoutLabel->AlignTo(mLSLabel, LV_ALIGN_OUT_BOTTOM_LEFT, 0, 15);

  mLSTimeOutDropDown->SetHeight(30);
  mLSTimeOutDropDown->SetWidth(GetContentWidth());
  mLSTimeOutDropDown->AddItem("10 Sec", (10 * 1000));
  mLSTimeOutDropDown->AddItem("1 Min", (1 * 60 * 1000));
  mLSTimeOutDropDown->AddItem("10 Min", (10 * 60 * 1000));
  mLSTimeOutDropDown->AddItem("30 Min", (30 * 60 * 1000));
  mLSTimeOutDropDown->AddItem("1 Hour", (60 * 60 * 1000));
  mLSTimeOutDropDown->AddItem("2 Hour", (120 * 60 * 1000));
  mLSTimeOutDropDown->AlignTo(mLSTimeoutLabel, LV_ALIGN_OUT_BOTTOM_MID);
  mLSTimeOutDropDown->SetSelected(
      HardwareFactory::getAbstract().getLightSleepTimeout());
}

SystemSettings::~SystemSettings() {
  if (mSaveReqrd)
    HardwareFactory::getAbstract().saveSettings();
}