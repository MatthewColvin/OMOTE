#include "BatteryRev1.hpp"
#include <Arduino.h>


BatteryRev1::BatteryRev1(int adc_pin, int charging_pin) : 
                                      mAdcPin(adc_pin),
                                      mChargingPin(charging_pin) {
  mAdcPin = adc_pin;
  mChargingPin = charging_pin;
  // Power Pin Definition
  pinMode(mChargingPin, INPUT_PULLUP);
  pinMode(mAdcPin, INPUT);
}

int BatteryRev1::getDirectSOC() {
  auto test = constrain(map(this->getVoltage(), 3700, 4200, 0, 100), 0, 100);
  return constrain(map(this->getVoltage(), 3700, 4200, 0, 100), 0, 100);
}

uint16_t BatteryRev1::getRawSOC() {
  return constrain(map(this->getVoltage(), 3400, 4250, 0, 100*256), 0, 100*256);
}

bool BatteryRev1::isCharging() {
  return !digitalRead(mChargingPin);
}

bool BatteryRev1::isConnected() {
  return ((!isCharging()) && (getVoltage() < 4350));
}

int BatteryRev1::getVoltage() {
  // 350mV ADC offset
  // adjusted values due to new measurements
  return analogRead(mAdcPin) * 2 * 3300 / 4095 + 325;
}