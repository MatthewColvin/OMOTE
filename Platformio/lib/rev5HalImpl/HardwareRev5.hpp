#pragma once

#include "HardwareRevX.hpp"
#include "BatteryRev5.hpp"
#include "Hardware/LoggingInterface.hpp"

class HardwareRev5 : public HardwareRevX {
public:
  HardwareRev5();
  virtual ~HardwareRev5() = default;

  std::shared_ptr<BatteryInterface> battery() override { return mBattery; };

protected:
  void init() override;
  void initIO() override;

private:
  void lightSleepWakeReint(SleepMode mode) override;

  void configIMUInterruptPolarity() override;
  void enableWakeupByPin() override;
  void sleepDisplayPins() override;

  void setupKeyboard();

  bool isUsbConnected() override;

  void setupLightSensor();

  bool keyboardScan() override;
  #ifdef OMOTE_KEYBRD_3661
  bool lightSensorScan(uint16_t &visPlusIrLevel, uint16_t &irLevel) override;
  #endif
  void updateBacklightMode(uint16_t lightLevel) override;

  // keypad scanning
  Adafruit_TCA8418 keypad;

#if defined(OMOTE_KEYBRD_3661)
  // light sensor
  Adafruit_LTR303 ltr = Adafruit_LTR303();
#endif

  //QueueHandle_t mKeysQueueHandle;

  bool mlightSensorInitSuccessful = false;

  std::shared_ptr<BatteryRev5> mBattery;

  std::unique_ptr<LoggingInterface> mLogger = nullptr;

  // Note: 'off' is not actually in matrix but dedicated pin, mapped to vacant
  // position in matrix for processing
#if defined(OMOTE_KEYBRD_3661)
  char indexToChar[KEYPAD_ROWS * KEYPAD_COLS] = {
      '+', '-', 'i',  //  volume+, volume-,    info,
      'L', 'b', 'o',  //     left,    back,     off,
      'y', 'm', 'k',  //    cycle,    mute,      OK,
      'h', '<', '=',  //     home,  rewind,    stop,
      '^', 'g', 'd',  // channel+,   guide,    down,
      'p', 'P', 'T',  //     play,   pause,      TV,
      'v', 'u', 'x',  // channel-,      up,    exit,
      'r', 'S', 'A',  //   record,  stream,   audio,
      'c', 'R', '>',  //   config,   right, forward,
      'B', 'D', 'Y'}; //      STB,     DVD,  BLURAY,
#else
  // Note: ? row/column entry is unused in hardware key matrix
  char indexToChar[KEYPAD_ROWS * KEYPAD_COLS] = {
      '?', 'p', 'c', '<', '=',  //       ?,     play,  config, rewind,   stop
      '>', 'o', 'b', 'u', 'L',  // forward,      off,    back,     up,   left
      '4', 'v', '1', '3', '2',  //    blue, channel-,     red, yellow,  green
      'i', 'R', '+', 'k', 'd',  //    info,    right, Volume+,     OK,   down
      's', '^', '-', 'm', 'r'}; //  source, channel+, Volume-,   mute, record
#endif
};
