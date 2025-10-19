#pragma once
#include "Hardware/BatteryInterface.h"
#include "Hardware/LoggingInterface.hpp"

class BatteryRevX : public BatteryInterface {
public:
  /**
   * @brief Get the Percentage of the battery
   *
   * @return int Percentage of the battery
   */
  virtual int getPercentage() override;

  /**
   * @brief Get the SOC of the battery
   *
   * @return int SOC of the battery
   */
  virtual int getDirectSOC() = 0;

  /**
   * @brief Get the raw soc value of the battery
   *
   * @return raw MAX17048 SOC register value
   */
  uint16_t getRawSOC() override = 0;

  /**
   * @brief Function to get the current voltage of the battery
   *
   * @return int Voltage of the battery in mV
   */
  virtual int getVoltage() override = 0;

  /**
   * @brief Function to determine if the battery is charging or not
   *
   * @return true   Battery is currently charging
   * @return false  Battery is currently not charging
   */
  virtual bool isCharging() override = 0;

  /**
   * @brief Function to save the battery linearisation data
   *
   * @param rawSOCs array of rawSOC values taken at regular intervals during charge
   * @param numVals number of rawSOC values
   */
  virtual void saveLinearisationData(bool charge, uint16_t *rawSOCs, uint16_t numVals, int startmV, int endmV) override;

  /**
   * @brief Function to write a custom battery model to the fuel gauge
   */
  virtual void writeCustomModel() override {};

  virtual void setCalMode(int mode) override;
  virtual int getCalMode() override { return mCalMode; };
  virtual std::vector<uint16_t> getCalData() override;

  /**
   * @brief Function to write a custom battery model to the fuel gauge
   */
  virtual void disableHibernate(bool disable) override {};

  /**
   * @brief Function to determine if the battery is connected
   *
   * @return true   Battery is connected
   * @return false  Battery is not connected
   */
  virtual bool isConnected() = 0;

  BatteryRevX(int adc_pin, int charging_pin);

  // Not sure why this is needed but shared_ptr seems to really
  // need it possibly a compiler template handling limitation
  // none the less we really should not use it.
  BatteryRevX() = default;

private:
  static constexpr auto maxBatCalVals = 100;

  std::vector<uint16_t> mCalValues;

protected:
  void loadLinearisationData(int);
  int loadCalMode();

  std::unique_ptr<LoggingInterface> mLogger = nullptr;
  mutable std::stringstream mLogStream;

  int mCalMode = 0;
};