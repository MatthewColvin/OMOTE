#include "BatteryRev5.hpp"
#include <SparkFun_MAX1704x_Fuel_Gauge_Arduino_Library.h>

//  Battery gas gauge
SFE_MAX1704X fuelGauge = SFE_MAX1704X(MAX1704X_MAX17048);

BatteryRev5::BatteryRev5(int adc_pin, int charging_pin) :
                            mChargingPin(charging_pin) {
  mLogger = std::make_unique<LoggingInterface>();                           
  mLogger->setLogModule(LogModule::Battery);
  loadLinearisationData(loadCalMode());
  // Power Pin Definition
  pinMode(mChargingPin, INPUT_PULLUP);
  if (!fuelGauge.begin()) {
    disableHibernate(false); //make sure running default hibernate settings
    mLogger->error("Couldn't find MAX17048 sensor!");
  }
}

int BatteryRev5::getDirectSOC() {
  float soc = fuelGauge.getSOC();
  if (soc > 100)
    soc = 100;
  return soc;
}

uint16_t BatteryRev5::getRawSOC() {
  uint16_t rawSoc = fuelGauge.read16(MAX17043_SOC);
  return rawSoc;
}

void BatteryRev5::disableHibernate(bool disable) {
  fuelGauge.write16(disable?0x0000:0x8030, MAX17048_HIBRT);
}

bool BatteryRev5::isCharging() {
  return digitalRead(mChargingPin) == LOW;
}

bool BatteryRev5::isConnected() {
  return ((!isCharging()) && (getVoltage() < 4350));
}

int BatteryRev5::getVoltage() {
  return 1000 * fuelGauge.getVoltage();
}

// from https://android.googlesource.com/kernel/tegra/+/android-tegra3-grouper-3.1-jb-mr1/drivers/power/max17048_battery.c
// appears to be for a Nexus 7 LiPo battery
#define MAX17048_UNLOCK 0x3E
#define MAX17048_TABLE 0x40
#define MAX17048_UNLOCK_VALUE 0x4a57
uint8_t max17048_custom_data1[] = {
    0xAA, 0x00, 0xB1, 0xF0, 0xB7, 0xE0, 0xB9, 0x60, 0xBB, 0x80,
    0xBC, 0x40, 0xBD, 0x30, 0xBD, 0x50, 0xBD, 0xF0, 0xBE, 0x40,
    0xBF, 0xD0, 0xC0, 0x90, 0xC4, 0x30, 0xC7, 0xC0, 0xCA, 0x60,
    0xCF, 0x30, 0x01, 0x20, 0x09, 0xC0, 0x1F, 0xC0, 0x2B, 0xE0,
    0x4F, 0xC0, 0x30, 0x00, 0x47, 0x80, 0x4F, 0xE0, 0x77, 0x00,
    0x15, 0x60, 0x46, 0x20, 0x13, 0x80, 0x1A, 0x60, 0x12, 0x20,
    0x14, 0xA0, 0x14, 0xA0};

uint8_t max17048_custom_data2[] = {
    0x9B, 0x70, 0xAB, 0x30, 0xB5, 0xA0, 0xB9, 0xD0, 0xBB, 0xA0,
    0xBC, 0x00, 0xBC, 0xB0, 0xBD, 0x00, 0xBD, 0x60, 0xBE, 0x40,
    0xBF, 0x40, 0xC1, 0xF0, 0xC5, 0x60, 0xC8, 0xA0, 0xCD, 0x00,
    0xD1, 0x50, 0x00, 0xE0, 0x01, 0x80, 0x18, 0x60, 0x1C, 0x20,
    0x54, 0x00, 0x6A, 0xC0, 0x79, 0x20, 0x65, 0xC0, 0x0B, 0xE0,
    0x2A, 0xC0, 0x1D, 0x00, 0x17, 0xE0, 0x15, 0xE0, 0x11, 0xE0,
    0x11, 0x00, 0x11, 0x00};

void BatteryRev5::writeCustomModel() {
  // unlock model access
  fuelGauge.write16(MAX17048_UNLOCK_VALUE, MAX17048_UNLOCK);

  // write model
  for (int i = 0; i < 32; i++) {
    uint16_t val = (((uint16_t)max17048_custom_data1[i * 2]) << 8) + max17048_custom_data1[(i * 2) + 1];
    fuelGauge.write16(val, MAX17048_TABLE + (2 * i));
  }

  // lock model access
  fuelGauge.write16(0x0000, MAX17048_UNLOCK);
}