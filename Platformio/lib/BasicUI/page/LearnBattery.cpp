#include "LearnBattery.hpp"
#include "Button.hpp"
#include "HardwareFactory.hpp"
#include "Label.hpp"
#include <iomanip>

using namespace UI::Page;

LearnBattery::LearnBattery()
    : Base(ID::Pages::LearnBattery),
      mBattery(HardwareFactory::getAbstract().battery()),
      mCalModeLabel(AddNewElement<Widget::Label>("SOC Calibration Mode")),
      mCalModeDropDown(AddNewElement<Widget::DropDown<int>>([this](int aMode) { 
        mBattery->setCalMode(aMode); 
        DrawGraph(mBattery->getCalData()); 
        mStart->SetDisabled(aMode == calMode::direct); })),
      // mStartCharge(AddNewElement<Widget::Button>([this] { StartCharge(); })),
      // mStartDischarge(AddNewElement<Widget::Button>([this] { StartDischarge(); })),
      mStart(AddNewElement<Widget::Button>([this] { Start(); })),
      mCharging(AddNewElement<Widget::Label>(LV_SYMBOL_CHARGE)),
      mVolts(AddNewElement<Widget::Label>("--mV")),
      mSOC(AddNewElement<Widget::Label>("--%")),
      mRawSOC(AddNewElement<Widget::Label>("--%")),
      mTabView(AddNewElement<Page::TabView>(ID(ID::Pages::INVALID_PAGE_ID))) {

  mCalModeLabel->SetHeight(15);
  mCalModeDropDown->SetHeight(30);
  mCalModeDropDown->SetWidth(lv_pct(80)); // GetContentWidth());
  mStart->SetHeight(lv_pct(10));
  mCharging->SetHeight(lv_pct(10));
  mCharging->SetWidth(lv_pct(14));
  mVolts->SetHeight(lv_pct(10));
  mVolts->SetWidth(lv_pct(28));
  mRawSOC->SetHeight(lv_pct(10));
  mRawSOC->SetWidth(lv_pct(28));
  mSOC->SetHeight(lv_pct(10));
  mSOC->SetWidth(lv_pct(22));
  mTabView->SetHeight(lv_pct(64));

  mCalModeDropDown->AddItem("MAX17048 SOC", calMode::direct);
  mCalModeDropDown->AddItem("Charge", calMode::charge);
  mCalModeDropDown->AddItem("Discharge", calMode::discharge);
  mCalModeDropDown->SetSelected(mBattery->getCalMode());

  mStart->SetText("Start Calibration");

  mCalModeLabel->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 0);
  mCalModeDropDown->AlignTo(mCalModeLabel, LV_ALIGN_OUT_BOTTOM_RIGHT);
  mStart->AlignTo(this, LV_ALIGN_TOP_LEFT, 0, 45);
  mCharging->AlignTo(mStart, LV_ALIGN_OUT_BOTTOM_LEFT, 0, distBetweenButtons);
  mVolts->AlignTo(mCharging, LV_ALIGN_OUT_RIGHT_BOTTOM, distBetweenButtons, 0);
  mRawSOC->AlignTo(mVolts, LV_ALIGN_OUT_RIGHT_BOTTOM, distBetweenButtons, 0);
  mSOC->AlignTo(mRawSOC, LV_ALIGN_OUT_RIGHT_BOTTOM, distBetweenButtons, 0);
  mTabView->AlignTo(this, LV_ALIGN_BOTTOM_LEFT, 0, -16); // alignment doesn't seem to take account of tabs

  auto mTextAreaPage = std::make_unique<Page::Base>(ID(ID::Pages::BatteryLog));
  mLog = mTextAreaPage->AddNewElement<Base>(lv_textarea_create(LvglSelf()),
                                            ID::Widgets::INVALID_WIDGET_ID);
  lv_obj_set_scrollbar_mode(mLog->LvglSelf(), LV_SCROLLBAR_MODE_AUTO);
  mTextAreaPage->SetTitle("Log");
  mTabView->AddTab(std::move(mTextAreaPage));

  auto mGraphPage = std::make_unique<Page::Base>(ID(ID::Pages::BatteryGraph));
  mGraph = mGraphPage->AddNewElement<Base>(lv_chart_create(LvglSelf()),
                                           ID::Widgets::INVALID_WIDGET_ID);
  lv_chart_set_range(mGraph->LvglSelf(), LV_CHART_AXIS_PRIMARY_Y, 0, 110);
  lv_chart_set_div_line_count(mGraph->LvglSelf(), 3, 5);
  DrawGraph(mBattery->getCalData());

  mGraphPage->SetTitle("Graph");
  mTabView->AddTab(std::move(mGraphPage));

  std::string log = "Press start to calibrate SOC curve.\n\n"
                    "Note: The process needs to do a full charge or discharge cycle which will take several hours during which the OMOTE should not be used.";
  AddToLog(log);
  lv_textarea_set_cursor_pos(mLog->LvglSelf(), 0);

  mStart->SetDisabled(mBattery->getCalMode() == calMode::direct);

  mTimer = lv_timer_create(LearnBattery::onTimer, 100, this);
}

LearnBattery::~LearnBattery() {
  mBattery->disableHibernate(false);
  if (mOrigSleepTime != 0) // if changed restore
    HardwareFactory::getAbstract().setSleepTimeout(mOrigSleepTime);
  if (mTimer) {
    lv_timer_del(mTimer);
    mTimer = nullptr;
  }
}

void LearnBattery::DrawGraph(const std::vector<uint16_t> &aData) {
  if (mSeries != nullptr) {
    lv_chart_remove_series(mGraph->LvglSelf(), mSeries);
    mSeries = nullptr;
  }
  lv_chart_set_point_count(mGraph->LvglSelf(), aData.size());
  mSeries = lv_chart_add_series(mGraph->LvglSelf(), lv_palette_main(LV_PALETTE_GREEN), LV_CHART_AXIS_PRIMARY_Y);
  for (int i = 0; i < aData.size(); i++)
    lv_chart_set_next_value(mGraph->LvglSelf(), mSeries, (int32_t)(aData[i] / 256));
  lv_chart_refresh(mGraph->LvglSelf());
}

void LearnBattery::Start() {
  if (mBattery->getCalMode() == calMode::charge)
    StartCharge();
  else if (mBattery->getCalMode() == calMode::discharge)
    StartDischarge();
}

void LearnBattery::StartCharge() {
  mLogStr.clear();
  mStartVoltage = mBattery->getVoltage();
  if (mBattery->isCharging())
    AddToLog("Please plug in the charger after starting the test\n");
#ifndef IS_SIMULATOR
  else if (mStartVoltage > 3500)
    AddToLog("Battery voltage too high, please discharge to less than 3500mV before starting test\n");
#endif
  else {
    auto logger = std::make_unique<LoggingInterface>();
    logger->setLogModule(LogModule::Battery);
    logger->info("Starting charge battery calibration at " + std::to_string(mStartVoltage) + "mV");
    mSocVals.clear();
    mStart->SetDisabled(true);
    mCalModeDropDown->SetDisabled(true);
    mLogStr.clear();
    AddToLog("Please connect charger\n");
    mState = WAIT_CHG;
  }
}

void LearnBattery::StartDischarge() {
  mLogStr.clear();
  mStartVoltage = mBattery->getVoltage();
  if (mBattery->isCharging())
    AddToLog("Please unplug the charger once fully charged and then start test\n");
#ifndef IS_SIMULATOR
  // else if (mStartVoltage < 4200)
  //   AddToLog("Battery voltage too low, please charge to above 4200mV before starting test\n");
#endif
  else {
    // save orig sleep time and change to ensure doesn't kick in during test
    mOrigSleepTime = HardwareFactory::getAbstract().getSleepTimeout();
    HardwareFactory::getAbstract().setSleepTimeout(900000); // 15min
    auto logger = std::make_unique<LoggingInterface>();
    logger->setLogModule(LogModule::Battery);
    logger->info("Starting discharge battery calibration at " + std::to_string(mStartVoltage) + "mV");
    mSocVals.clear();
    mStart->SetDisabled(true);
    mCalModeDropDown->SetDisabled(true);
    mLogStr.clear();
    mState = WAIT_DISCHG;
  }
}

void LearnBattery::AddToLog(std::string aLogEntry) {
  mLogStr += aLogEntry;
  lv_textarea_set_text(mLog->LvglSelf(), mLogStr.c_str());
}

void LearnBattery::onTimer(_lv_timer_t *aTimer) {
  LearnBattery *currentLearnBattery =
      reinterpret_cast<LearnBattery *>(lv_timer_get_user_data(aTimer));

  static int pass = 0;
  static int wakeCount = 20;
  static unsigned long expectedWakeTime = 0;
  static HardwareAbstract::WakeReason wakeReason = HardwareAbstract::WakeReason::TIMER;
  static uint16_t preRelaxSoc = 0;
  static int mvLowCount = 0;

  int millivolts = currentLearnBattery->mBattery->getVoltage();
  uint16_t rawSoc = currentLearnBattery->mBattery->getRawSOC();

  currentLearnBattery->mVolts->SetText(std::to_string(millivolts) + "mV");
  std::stringstream stream;
  stream << std::fixed << "r:" << std::setprecision(1) << rawSoc / 256.0f << "%";
  std::string s = stream.str();
  currentLearnBattery->mRawSOC->SetText(s);
  currentLearnBattery->mSOC->SetText("c:" + std::to_string(currentLearnBattery->mBattery->getPercentage()) + "%");

  auto logger = std::make_unique<LoggingInterface>();
  logger->setLogModule(LogModule::Battery);

  if (currentLearnBattery->mState != MEASURE_DISCHG)
    currentLearnBattery->mCharging->SetText(currentLearnBattery->mBattery->isCharging() ? LV_SYMBOL_CHARGE : "");

  switch (currentLearnBattery->mState) {
  case IDLE:
  default:
    pass = 0;
    break;
  case WAIT_CHG:
#ifdef IS_SIMULATOR
    if (true) {
#else
    if (currentLearnBattery->mBattery->isCharging()) {
#endif
      currentLearnBattery->mBattery->disableHibernate(true);
      wakeReason = HardwareAbstract::WakeReason::TIMER;
      currentLearnBattery->mState = MEASURE_CHG;
      currentLearnBattery->mLogStr.clear();
      currentLearnBattery->AddToLog("Charger connected, starting calibration, sleeping for 10min\n");
      wakeCount = 50;
    }
    break;
  case MEASURE_CHG:
    if (wakeReason == HardwareAbstract::WakeReason::TIMER) {
      // do measurement
      if (wakeCount-- == 0) {
        currentLearnBattery->mSocVals.push_back(rawSoc);

        currentLearnBattery->DrawGraph(currentLearnBattery->mSocVals);

        std::stringstream stream;
        stream << "\n#" << std::to_string(pass++) << ", " << millivolts << "mV, SOC:0x" << std::hex << rawSoc;
        std::string logString(stream.str());

        currentLearnBattery->AddToLog(logString);
        lv_textarea_set_cursor_pos(currentLearnBattery->mLog->LvglSelf(), LV_TEXTAREA_CURSOR_LAST);

        logger->info(logString);

        if (currentLearnBattery->mSocVals.size() < 100) {
          expectedWakeTime = HardwareFactory::getAbstract().getMillis() + 600000;
          HardwareFactory::getAbstract().enterSleep(HardwareAbstract::SleepMode::LIGHT_SLEEP_WAKE_ON_NOCHG, 600000); // wake every 10min
          wakeReason = HardwareFactory::getAbstract().getWakeUpReason();
          wakeCount = 50;
        } else {
          currentLearnBattery->mLogStr.clear();
          currentLearnBattery->AddToLog("Too many data points, calibration cancelled\n");
          currentLearnBattery->setCalComplete();
        }
      }
    } else if (wakeReason == HardwareAbstract::WakeReason::CHARGER) {
      if (millivolts < 4150) {
        currentLearnBattery->mLogStr.clear();
        currentLearnBattery->AddToLog("Battery not fully charged, calibration cancelled\n");
      } else {
        if (currentLearnBattery->mSocVals.size() > 10) {
          // combine the last three measurements to allow for tapering current at end of charge
          currentLearnBattery->mSocVals.resize(currentLearnBattery->mSocVals.size() - 2); //-2 as this one not added yet
          currentLearnBattery->mSocVals.push_back(rawSoc);
          currentLearnBattery->mBattery->saveLinearisationData(true, &currentLearnBattery->mSocVals[0], currentLearnBattery->mSocVals.size(), currentLearnBattery->mStartVoltage, millivolts);
          currentLearnBattery->mLogStr.clear();
          currentLearnBattery->AddToLog("Complete at " + std::to_string(millivolts) + "mV, data saved\n");
          logger->info("Battery calibration complete at " + std::to_string(millivolts) + "mV");
        } else {
          currentLearnBattery->mLogStr.clear();
          currentLearnBattery->AddToLog("Too few data points, calibration cancelled\n");
        }
      }
      currentLearnBattery->setCalComplete();
    } else {
      // if woken by key or IMU go back to sleep after delay
      if (wakeCount-- == 0) {
        auto sleepRem = expectedWakeTime - HardwareFactory::getAbstract().getMillis() - 5000; // 5000 allows for time consumed by wake
        if (sleepRem > 0) {                                                                   // if sleep time left go back to sleep
          logger->info("Woke due to keyboard or IMU, sleeping for another" + std::to_string(sleepRem) + "ms");
          HardwareFactory::getAbstract().enterSleep(HardwareAbstract::SleepMode::LIGHT_SLEEP_WAKE_ON_NOCHG, sleepRem);
          wakeReason = HardwareFactory::getAbstract().getWakeUpReason();
        } else // otherwise trigger measure on next call to onTimer
          wakeReason = HardwareAbstract::WakeReason::TIMER;
        wakeCount = 50;
      }
    }
    break;
  case WAIT_DISCHG:
    currentLearnBattery->mBattery->disableHibernate(true);
    currentLearnBattery->mLogStr.clear();
    currentLearnBattery->AddToLog("Starting test, please ensure charger is unplugged.\nSleeping for 2min to relax battery\n");
    currentLearnBattery->mState = RELAX;
    wakeCount = 50;
    break;
  case RELAX:
    // This is here to allow the battery voltage to relax before the measurement is made to better reflect
    // the OMOTE normal operating mode (brief periods of use between longer periods of inactivity).
    // However, the MAX17048 is very good at masking this and the change in voltage seen (~40mV)
    // has very little, if any, effect on the SOC so it probably isn't needed.
    // May work well with Rev4 or less hardware to minearise mV based SOC but not yet tested
    if (wakeCount-- == 0) {
      currentLearnBattery->mState = MEASURE_DISCHG;
      preRelaxSoc = rawSoc;
      expectedWakeTime = HardwareFactory::getAbstract().getMillis() + 120000;
      HardwareFactory::getAbstract().enterSleep(HardwareAbstract::SleepMode::LIGHT_SLEEP_WAKE_ON_CHG, 120000); // 2min
      wakeReason = HardwareFactory::getAbstract().getWakeUpReason();
    } else {
      if (millivolts < 3500) {
        if (++mvLowCount >= 300) { // if below thresh for 30sec continuously
          if (currentLearnBattery->mSocVals.size() > 10) {
            std::reverse(currentLearnBattery->mSocVals.begin(), currentLearnBattery->mSocVals.end());
            currentLearnBattery->mBattery->saveLinearisationData(false, &currentLearnBattery->mSocVals[0], currentLearnBattery->mSocVals.size(), currentLearnBattery->mStartVoltage, millivolts);
            currentLearnBattery->mLogStr.clear();
            currentLearnBattery->AddToLog("Complete at " + std::to_string(millivolts) + "mV, data saved\n");
            logger->info("Battery calibration complete at " + std::to_string(millivolts) + "mV");
          } else {
            currentLearnBattery->mLogStr.clear();
            currentLearnBattery->AddToLog("Too few data points, calibration cancelled\n");
          }
          HardwareFactory::getAbstract().setSleepTimeout(currentLearnBattery->mOrigSleepTime);
          currentLearnBattery->setCalComplete();
        }
      } else
        mvLowCount = 0;
    }
    currentLearnBattery->mCharging->SetText(std::to_string(wakeCount / 10));
    break;
  case MEASURE_DISCHG:
    if (wakeReason == HardwareAbstract::WakeReason::TIMER) {
      std::stringstream stream;
      stream << "\n#" << std::to_string(pass++) << ", Pre:0x" << std::hex << preRelaxSoc << ", Post:0x" << rawSoc;
      std::string logString(stream.str());

      currentLearnBattery->AddToLog(logString);
      lv_textarea_set_cursor_pos(currentLearnBattery->mLog->LvglSelf(), LV_TEXTAREA_CURSOR_LAST);

      logger->info(logString);
      if (currentLearnBattery->mSocVals.size() < 100) {
        // save measurement
        currentLearnBattery->mSocVals.push_back(rawSoc);
        currentLearnBattery->DrawGraph(currentLearnBattery->mSocVals);
        currentLearnBattery->mState = RELAX;
        wakeCount = 6000; // 10min
      } else {
        currentLearnBattery->mLogStr.clear();
        currentLearnBattery->AddToLog("Too many data points, calibration cancelled\n");
        HardwareFactory::getAbstract().setSleepTimeout(currentLearnBattery->mOrigSleepTime);
        currentLearnBattery->setCalComplete();
      }
    } else if (wakeReason == HardwareAbstract::WakeReason::CHARGER) {
      currentLearnBattery->mLogStr.clear();
      currentLearnBattery->AddToLog("Charger plugged in, calibration cancelled\n");
      HardwareFactory::getAbstract().setSleepTimeout(currentLearnBattery->mOrigSleepTime);
      currentLearnBattery->setCalComplete();
    } else {
      // if woken by key or IMU go back to sleep after delay
      if (wakeCount-- == 0) {
        auto sleepRem = expectedWakeTime - HardwareFactory::getAbstract().getMillis() - 5000; // 5000 allows for time consumed by wake
        if (sleepRem > 0) {                                                                   // if sleep time left go back to sleep
          logger->info("Woke due to keyboard or IMU, sleeping for another" + std::to_string(sleepRem) + "ms");
          HardwareFactory::getAbstract().enterSleep(HardwareAbstract::SleepMode::LIGHT_SLEEP_WAKE_ON_CHG, sleepRem);
          wakeReason = HardwareFactory::getAbstract().getWakeUpReason();
        } else // otherwise trigger measure on next call to onTimer
          wakeReason = HardwareAbstract::WakeReason::TIMER;
        wakeCount = 50;
      }
    }
    break;
  }
}

void LearnBattery::setCalComplete() {
  mState = IDLE;
  mBattery->disableHibernate(false);
  mStart->SetDisabled(false);
  mCalModeDropDown->SetDisabled(false);
}