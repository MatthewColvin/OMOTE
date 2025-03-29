#include "battery.hpp"
#include <Arduino.h>

#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

//  Battery gas gauge
SFE_MAX1704X fuelGauge = SFE_MAX1704X(MAX1704X_MAX17048);

Battery::Battery(int adc_pin, int charging_pin): BatteryInterface(),
    mAdcPin(adc_pin),
    mChargingPin(charging_pin)
{
    // Power Pin Definition
    pinMode(mChargingPin, INPUT_PULLUP);
    if (!fuelGauge.begin()) Serial.println("Couldn't find MAX17048 sensor!");
}

int Battery::getPercentage()
{
  int soc = (int)fuelGauge.getSOC();
  if(soc > 99) soc = 99;
  return soc;
}

bool Battery::isCharging()
{
    return !digitalRead(mChargingPin);
}

bool Battery::isConnected()
{
    return ((!isCharging()) && (getVoltage() < 4350));
}

int Battery::getVoltage()
{
  return fuelGauge.getVoltage();
}