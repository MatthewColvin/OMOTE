#include "BatteryRevX.hpp"
#include <Preferences.h>

BatteryRevX::BatteryRevX(int adc_pin, int charging_pin) : BatteryInterface(),
                                                          mLogger(std::make_unique<LoggingInterface>()) {
  mLogger->setLogModule(LogModule::Battery);
  loadLinearisationData(loadCalMode());
}

int BatteryRevX::getPercentage() {
  float soc = 0;
  if (mCalValues.size() == 0) { // use MAX17048 soc directly
    soc = getDirectSOC();
    if (mLogger) {
      mLogStream << "SOC:" << soc;
      mLogger->debug(mLogStream);
    }
  } else { // use cal data
    uint16_t rawSoc = getRawSOC();
    if (rawSoc <= mCalValues.front()) {
      mLogger->debug("rawSOC:" + std::to_string(rawSoc) + ", SOC:<0%");
      return 0; // empty
    } else if (rawSoc >= mCalValues.back()) {
      mLogger->debug("rawSOC:" + std::to_string(rawSoc) + ", SOC:>100%");
      return 100; // full
    } else {
      int i = 0;
      // binary search would be quicker but this will do!
      for (i = 0; i < mCalValues.size() - 1; i++)
        if (rawSoc < mCalValues[i + 1])
          break;

      float incPerVal = 100.0f / (mCalValues.size() - 1);
      float socUnits = (incPerVal * i);
      float socFrac = (incPerVal * (((float)(rawSoc - mCalValues[i])) / (mCalValues[i + 1] - mCalValues[i])));
      soc = socUnits + socFrac;
      if (mLogger && mLogger->isPrintWanted(LogLevel::Debug)) {
        mLogStream << "rawSOC:" << rawSoc << ", SOC:" << soc << ", Units:" << socUnits << ", Fract:" << socFrac;
        mLogger->log(LogLevel::Debug, mLogStream);
      }
    }
  }
  return (int)soc;
}

void BatteryRevX::saveLinearisationData(bool charge, uint16_t *rawSOCs, uint16_t numVals, int startmV, int endmV) {
  if (numVals < maxBatCalVals) {
    Preferences preferences;
    preferences.begin(charge ? "batChgCurve" : "batDisCurve", false);
    preferences.putBytes("data", rawSOCs, numVals * sizeof(uint16_t));
    preferences.putInt("startmV", startmV);
    preferences.putInt("endmV", endmV);
    preferences.end();

    mLogger->debug("Battery calibration data saved");
  } else
    mLogger->error("Too many cal values, no data saved");
}

void BatteryRevX::setCalMode(int mode) {
  mCalMode = mode;
  Preferences preferences;
  preferences.begin("batCalMode", false);
  preferences.putInt("mode", mode);
  preferences.end();
  loadLinearisationData(mCalMode);
}

std::vector<uint16_t> BatteryRevX::getCalData() {
  if (mCalMode == calMode::direct)
    return std::vector<uint16_t>();
  else
    return mCalValues;
}

void BatteryRevX::loadLinearisationData(int mode) {
  mCalValues.clear();
  if (mode == calMode::direct) {
    mLogger->info("Using dirext MAX17048 SOC data");
  } else {
    Preferences preferences;
    preferences.begin(mode == calMode::charge ? "batChgCurve" : "batDisCurve", true);
    auto numBytes = preferences.getBytesLength("data");
    size_t numVals = numBytes / sizeof(uint16_t);
    if ((numVals > 10) && (numVals < maxBatCalVals)) {
      mLogger->debug("Start volts:" + std::to_string(preferences.getInt("startmV")) + "mV");
      mLogger->debug("End volts:" + std::to_string(preferences.getInt("endmV")) + "mV");
      uint16_t buffer[maxBatCalVals];
      preferences.getBytes("data", buffer, sizeof(buffer));
      for (int i = 0; i < numVals; i++) {
        mCalValues.push_back(buffer[i]);
        mLogger->debug(std::to_string(buffer[i]));
      }

      if (mLogger->isPrintWanted(LogLevel::Info)) {
        mLogStream << "Loaded " << mCalValues.size() << " calibration values";
        mLogger->log(LogLevel::Info, mLogStream);
      }
    } else
      mLogger->info("No battery cal data found");

    preferences.end();
  }
}

int BatteryRevX::loadCalMode() {
  Preferences preferences;
  preferences.begin("batCalMode", true);
  mCalMode = preferences.getInt("mode", calMode::direct);
  preferences.end();
  return mCalMode;
}