#include <chrono>
#include <cmath>

#include "Hardware/BatteryInterface.h"
#include "HardwareFactory.hpp"
#include "observerHandles.hpp"

#define OBS_BUF_SIZE 10

class BatterySimulator : public BatteryInterface {
public:
  BatterySimulator()
      : mCreationTime(std::chrono::high_resolution_clock::now()) {}

  ~BatterySimulator() {}

  virtual int getPercentage() override {
    auto now = std::chrono::high_resolution_clock::now();
    auto batteryRunTime =
        std::chrono::duration_cast<std::chrono::seconds>(now - mCreationTime);
    constexpr auto minToBatteryZero = 3;
    mfakeBattPercentage =
        100 - ((batteryRunTime / std::chrono::duration<float, std::ratio<60LL>>(
                                     minToBatteryZero)) *
               100);
    if (mfakeBattPercentage < 0)
      mfakeBattPercentage = 0;

    return std::floor(mfakeBattPercentage <= 100 ? mfakeBattPercentage : 0);
  }

  virtual uint16_t getRawSOC() override { return (mfakeBattPercentage * 256); }

  virtual int getVoltage() override { return 3600; }

  virtual bool isCharging() override { return false; }

  virtual void writeCustomModel() override {}
  virtual void disableHibernate(bool disable) override {}
  virtual void saveLinearisationData(bool charge, uint16_t *rawSOCs, uint16_t numVals, int startmV, int endmV) override {}
  virtual void setCalMode(int mode) override { mCalMode = mode; }
  virtual int getCalMode() override { return mCalMode; }

  virtual std::vector<uint16_t> getCalData() override {
    std::vector<uint16_t> data;
    if (mCalMode == calMode::charge)
      data = chargeData;
    else if (mCalMode == calMode::discharge)
      data = dischargeData;

    // for (uint16_t &d : data)
    //   d += (5.0 * 256 * mCalMode);

    return data;
  }

private:
  const std::vector<uint16_t> chargeData = {
      277,
      709,
      1209,
      1724,
      2228,
      2758,
      3487,
      4282,
      5363,
      6573,
      8113,
      9621,
      11172,
      12803,
      14416,
      15897,
      17118,
      18160,
      19168,
      20170,
      21089,
      21884,
      22545,
      23219,
      23948,
      24636,
      25203,
      25932};
  const std::vector<uint16_t> dischargeData = {
      392,
      477,
      796,
      1819,
      3354,
      5011,
      6793,
      8333,
      9621,
      10858,
      11905,
      12901,
      13849,
      14659,
      15388,
      16008,
      16637,
      17303,
      17992,
      18664,
      19294,
      19878,
      20546,
      21339,
      22362,
      23583,
      24920,
      26013};
  std::chrono::_V2::system_clock::time_point mCreationTime;
  int mCalMode = 0;
  float mfakeBattPercentage = 0.0f;
};