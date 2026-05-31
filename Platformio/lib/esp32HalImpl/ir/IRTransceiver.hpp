#pragma once
#include <IRrecv.h>
#include <IRremoteESP8266.h>
#include <IRsend.h>
#include <freertos/task.h>

#include "Hardware/IRInterface.h"
#include "Hardware/LoggingInterface.hpp"

#define MAX_MESSAGE_SIZE 800 //pronto message can be 522bytes!

class IRTransceiver : public IRInterface, protected IRsend, protected IRrecv {

public:
  IRTransceiver(std::unique_ptr<LoggingInterface> aLogger);
  virtual ~IRTransceiver();

  void send(int64SendTypes protocol, uint64_t data) override;
  void send(int16SendTypes protocol, std::vector<uint16_t> &data, uint16_t repeat) override;
  void send(constInt64SendTypes protocol, const uint64_t data) override;
  void send(charArrSendType protocol, const unsigned char data[]) override;
  void send(IRInterface::RawIR aRawIr) override;
  void sendBackground(std::string protocol, std::vector<std::string> data) override;

  int8_t calibrateTx() override {
    maxOutTaskPriority();
    auto calibrationOffset = IRsend::calibrate();
    restoreTaskPriority();
    Serial.printf("Calibration Offset: %i\r\n", calibrationOffset);
    return calibrationOffset;
  };

  void enableRx() override;
  void disableRx() override;
  void loopHandleRx() override;

private:
  static void IRSendTask(void *aStruct);
  void maxOutTaskPriority();
  void restoreTaskPriority();
  BaseType_t mPreSendPriority = 0;

  bool mIsRxEnabled = false;
  decode_results mCurrentResults;

  struct IrCaptureInfo {
    bool valid = false;
    std::string protocol;
    std::string dataHex;
    std::string human;
  };
  IrCaptureInfo mLastCapture;

public:
  const IrCaptureInfo &lastCapture() const { return mLastCapture; }
  void clearLastCapture() { mLastCapture = {}; }

private:
  TaskHandle_t mIRSendTask;
  std::unique_ptr<LoggingInterface> mLog = nullptr;
};