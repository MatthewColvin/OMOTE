#pragma once
#include "Hardware/IRInterface.h"

class IRSim : public IRInterface {
  void send(int64SendTypes protocol, uint64_t data) override;
  void send(int16SendTypes protocol, std::vector<uint16_t> &data, uint16_t repeat) override;
  void send(constInt64SendTypes protocol, const uint64_t data) override;
  void send(charArrSendType protocol, const unsigned char data[]) override;
  void send(RawIR aRawIr) override;
  void sendBackground(std::string protocol, std::vector<std::string> data) override;

  int8_t calibrateTx() override;

  void enableRx() override;
  void disableRx() override;
  void loopHandleRx() override;
};