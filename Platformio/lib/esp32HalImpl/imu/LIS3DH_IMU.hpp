#pragma once

#include "SparkFunLIS3DH.h"

#include <memory>

class LoggingInterface;

class LIS3DH_IMU {
public:
  LIS3DH_IMU(LIS3DH &aIMU);
  virtual ~LIS3DH_IMU() = default;

  void setup();

  bool activityDetection();

  void configIMUInterrupts(bool aWakeupByIMUEnabled);

  void configIMUInterruptPolarity();

private:
  LIS3DH &mIMU;
  // TODO: construct this here eventually but for now just get ref
  //       so integration strain is lower
  // LIS3DH(I2C_MODE, 0x19); // Default constructor is I2C, addr 0x19.
  std::unique_ptr<LoggingInterface> mLogger = nullptr;
};