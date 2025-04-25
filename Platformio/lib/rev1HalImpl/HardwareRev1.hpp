#pragma once

#include "HardwareRevX.hpp"
#include <Keypad.h> // modified for inverted logic

class HardwareRev1 : public HardwareRevX {
public:
  HardwareRev1() = default;
  virtual ~HardwareRev1() = default;

  void init() override;

private:
  void initIO() override;

  void sleepDisplayPins() override;
  void configPinsForSleepInterrupts() override;
  bool keyboardScan() override;

  // Keypad declarations
  static const byte ROWS = KEYPAD_ROWS; // 5;  // four rows
  static const byte COLS = KEYPAD_COLS; // 5;  // four columns
  // define the symbols on the buttons of the keypads
    char hexaKeys[ROWS][COLS] = {
        {'s', '^', '-', 'm', 'r'}, //  source, channel+, Volume-,   mute, record
        {'i', 'R', '+', 'k', 'd'}, //    info,    right, Volume+,     OK,   down
        {'4', 'v', '1', '3', '2'}, //    blue, channel-,     red, yellow,  green
        {'>', 'o', 'b', 'u', 'L'}, // forward,      off,    back,     up,   left
        {'?', 'p', 'c', '<', '='}  //       ?,     play,  config, rewind,   stop
    };
  // Note: ? row/column entry is unused in hardware key matrix

  byte rowPins[ROWS] = {SW_A, SW_B, SW_C, SW_D,
                        SW_E}; // connect to the row pinouts of the keypad
  byte colPins[COLS] = {SW_1, SW_2, SW_3, SW_4,
                        SW_5}; // connect to the column pinouts of the keypad
  Keypad customKeypad =
      Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
};
