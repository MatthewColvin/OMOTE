#pragma once
#include "DropDown.hpp"
#include "Hardware/BatteryInterface.h"
#include "Notification.hpp"
#include "PageBase.hpp"
#include "TabView.hpp"
namespace UI::Widget {
class Label;
class Button;
} // namespace UI::Widget

namespace UI::Page {

class LearnBattery : public Base {
public:
  LearnBattery();
  virtual ~LearnBattery();

  std::string GetTitle() override { return "Battery"; };

private:
  enum States { IDLE,
                WAIT_CHG,
                MEASURE_CHG,
                WAIT_DISCHG,
                RELAX,
                MEASURE_DISCHG };
  void Start();
  void StartCharge();
  void StartDischarge();
  void AddToLog(std::string aLogEntry);
  void DrawGraph(const std::vector<uint16_t> &aData);
  void setCalComplete();
  static void onTimer(_lv_timer_t *aTimer);

  std::shared_ptr<BatteryInterface> mBattery;

  static constexpr auto distBetweenButtons = 5;
  lv_timer_t *mTimer = nullptr;
  std::string mLogStr;
  Widget::Label *mCalModeLabel;
  Widget::DropDown<int> *mCalModeDropDown;
  Widget::Button *mStart;
  Widget::Label *mCharging;
  Page::TabView *mTabView;
  Base *mLog;
  Base *mGraph;
  Widget::Label *mVolts;
  Widget::Label *mSOC;
  Widget::Label *mRawSOC;

  int mStartVoltage = 0;
  uint32_t mOrigSleepTime = 0;
  std::vector<uint16_t> mSocVals;
  States mState = IDLE;
  lv_chart_series_t *mSeries = nullptr;
};

} // namespace UI::Page
