#if defined(OMOTE_HARDWARE_REV5)

#pragma once

#include "HardwareRev1.hpp"

class HardwareRev5 : public HardwareRev1 {
 public:
  HardwareRev5() = default;
  virtual ~HardwareRev5() = default;

 protected:
  void init() override;

 private:
  // Setup IMU for active high interrupt
  void configIMUInterruptPolarity() override;
  void enableWakeupByPin() override;

  void setupKeyboard();

  void keyboardScan() override;

  // keypad scanning
  Adafruit_TCA8418 keypad;

  QueueHandle_t mKeysQueueHandle;
  char indexToChar[KEYPAD_ROWS * KEYPAD_COLS] = {
      '+', '-', 'i',
      'L', 'b', 'o',  // volume+, volume-,    info,    left,  back,  NotUsed
      't', 'm', 'k',
      'h', '<', '=',  //  return,    mute,      OK,    home,  rewind,  stop,
      '^', 'g', 'd',
      'p', 's', 'T',  // channel+,   guide,    down,    play,   pause,  TV
      'v', 'u', 'x',
      'r', 'S', 'A',  // channel-,      up,    exit,  record,  stream,  audio
      'c', 'R', '>',
      'B', 'D', 'Y'};  //    config,   right, forward,     STB,     DVD,  BLURAY
};

#endif
