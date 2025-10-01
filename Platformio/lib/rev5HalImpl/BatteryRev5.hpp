#pragma once
#include "BatteryRevX.hpp"
#include "Hardware/LoggingInterface.hpp"

class BatteryRev5 : public BatteryRevX {
public:
  /**
   * @brief Get the SOC of the battery
   *
   * @return int SOC of the battery
   */
  int getDirectSOC() override;

  /**
   * @brief Get the raw soc value of the battery
   *
   * @return raw MAX17048 SOC register value
   */
  uint16_t getRawSOC() override;

  /**
   * @brief Function to get the current voltage of the battery
   *
   * @return int Voltage of the battery in mV
   */
  int getVoltage() override;

  /**
   * @brief Function to write a custom battery model to the fuel gauge
   */
  void disableHibernate(bool disable) override;

  /**
   * @brief Function to determine if the battery is charging or not
   *
   * @return true   Battery is currently charging
   * @return false  Battery is currently not charging
   */
  bool isCharging() override;

  /**
   * @brief Function to determine if the battery is connected
   *
   * @return true   Battery is connected
   * @return false  Battery is not connected
   */
  bool isConnected() override;

  void writeCustomModel();

  BatteryRev5(int adc_pin, int charging_pin);

  // Not sure why this is needed but shared_ptr seems to really
  // need it possibly a compiler template handling limitation
  // none the less we really should not use it.
  BatteryRev5() = default;

private:
  static constexpr auto maxBatCalVals = 100;

  /**
   * @brief Variable to store which pin is used to indicate if the battery is
   * currently charging or not
   *
   */
  int mChargingPin;

  using BatteryRevX::mLogger;
  using BatteryRevX::mLogStream;
};