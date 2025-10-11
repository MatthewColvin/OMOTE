#include "LIS3DH_IMU.hpp"
#include "Hardware/LoggingInterface.hpp"
#include "omoteconfig.h"

LIS3DH_IMU::LIS3DH_IMU(LIS3DH &aIMU) : mIMU(aIMU), mLogger(std::make_unique<LoggingInterface>(LogModule::IMU)) {
}

void LIS3DH_IMU::setup() {
  // Setup hal
  // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
  // Note: 10ms per sample allows guaranteed clean sample between I2C reads
  mIMU.settings.accelSampleRate = 100;
  // Max G force readable.  Can be: 2, 4, 8, 16
  mIMU.settings.accelRange = 2;
  mIMU.settings.adcEnabled = 0;
  mIMU.settings.tempEnabled = 0;
  mIMU.settings.xAccelEnabled = 1;
  mIMU.settings.yAccelEnabled = 1;
  mIMU.settings.zAccelEnabled = 1;
  mIMU.begin();
  uint8_t intDataRead;
  mIMU.readRegister(&intDataRead, LIS3DH_INT1_SRC); // clear interrupt
}

bool LIS3DH_IMU::activityDetection() {
  bool activityDetected = false;
  static int accXold = mIMU.readFloatAccelX() * 1000;
  static int accYold = mIMU.readFloatAccelY() * 1000;
  static int accZold = mIMU.readFloatAccelZ() * 1000;
  static int accXbuf[4], accYbuf[4], accZbuf[4];
  static int motion = 0;
  static uint8_t bufferIndex = 0;

  accXbuf[bufferIndex] = mIMU.readFloatAccelX() * 1000;
  accYbuf[bufferIndex] = mIMU.readFloatAccelY() * 1000;
  accZbuf[bufferIndex] = mIMU.readFloatAccelZ() * 1000;

  bufferIndex++;
  if (bufferIndex >= 4) {
    bufferIndex = 0;
    int accX = (accXbuf[0] + accXbuf[1] + accXbuf[2] + accXbuf[3]) / 4;
    int accY = (accYbuf[0] + accYbuf[1] + accYbuf[2] + accYbuf[3]) / 4;
    int accZ = (accZbuf[0] + accZbuf[1] + accZbuf[2] + accZbuf[3]) / 4;
    // determine motion value as da/dt
    motion = (abs(accXold - accX) + abs(accYold - accY) + abs(accZold - accZ));
    // Store the current acceleration and time
    accXold = accX;
    accYold = accY;
    accZold = accZ;
    if (motion > MOTION_THRESHOLD)
      activityDetected = true;

    if (mLogger && mLogger->isPrintWanted(LogLevel::Debug)) {
      std::stringstream ss;
      ss << "Motion Level :" << motion << ", Detected: " << activityDetected;
      mLogger->log(LogLevel::Debug, ss);
    }
  }
  return activityDetected;
}

void LIS3DH_IMU::configIMUInterrupts(bool aWakeupByIMUEnabled) {
  uint8_t dataToWrite = 0;

  // LIS3DH_INT1_CFG
  // dataToWrite |= 0x80;//AOI, 0 = OR 1 = AND
  // dataToWrite |= 0x40;//6D, 0 = interrupt source, 1 = 6 direction source
  // Set these to enable individual axes of generation source (or direction)
  //  -- high and low are used generically
  dataToWrite |= 0x20; // Z high
  // dataToWrite |= 0x10;//Z low
  dataToWrite |= 0x08; // Y high
  // dataToWrite |= 0x04;//Y low
  dataToWrite |= 0x02; // X high
  // dataToWrite |= 0x01;//X low
  if (aWakeupByIMUEnabled)
    mIMU.writeRegister(LIS3DH_INT1_CFG, 0b00101010);
  else
    mIMU.writeRegister(LIS3DH_INT1_CFG, 0b00000000);

  // LIS3DH_INT1_THS
  dataToWrite = 0;
  // Provide 7 bit value, 0x7F always equals max range by accelRange setting
  dataToWrite |= 0x45;
  mIMU.writeRegister(LIS3DH_INT1_THS, dataToWrite);

  // LIS3DH_INT1_DURATION
  dataToWrite = 0;
  // minimum duration of the interrupt
  // LSB equals 1/(sample rate)
  dataToWrite |= 0x00; // 1 * 1/50 s = 20ms
  mIMU.writeRegister(LIS3DH_INT1_DURATION, dataToWrite);

  // LIS3DH_CTRL_REG5
  // Int1 latch interrupt and 4D on  int1 (preserve fifo en)
  mIMU.readRegister(&dataToWrite, LIS3DH_CTRL_REG5);
  dataToWrite &= 0xF3; // Clear bits of interest
  dataToWrite |= 0x08; // Latch interrupt (Cleared by reading int1_src)
  // dataToWrite |= 0x04; //Pipe 4D detection from 6D recognition to int1?
  mIMU.writeRegister(LIS3DH_CTRL_REG5, dataToWrite);

  // LIS3DH_CTRL_REG6
  configIMUInterruptPolarity();

  // LIS3DH_CTRL_REG3
  // Choose source for pin 1
  dataToWrite = 0;
  // dataToWrite |= 0x80; //Click detect on pin 1
  dataToWrite |= 0x40; // AOI1 event (Generator 1 interrupt on pin 1)
  dataToWrite |= 0x20; // AOI2 event ()
  // dataToWrite |= 0x10; //Data ready
  // dataToWrite |= 0x04; //FIFO watermark
  // dataToWrite |= 0x02; //FIFO overrun
  mIMU.writeRegister(LIS3DH_CTRL_REG3, dataToWrite);
}

void LIS3DH_IMU::configIMUInterruptPolarity() {
#if defined(OMOTE_HARDWARE_) || defined(OMOTE_HARDWARE_REV6)
  mIMU.writeRegister(LIS3DH_CTRL_REG6, 0x02); // For active-low interrupt
#else
  mIMU.writeRegister(LIS3DH_CTRL_REG6, 0x00); // For active-high interrupt
#endif
}