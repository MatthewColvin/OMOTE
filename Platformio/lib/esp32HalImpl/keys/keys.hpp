#pragma once
#include <Keypad.h> // modified for inverted logic

#include <map>

#include "Hardware/KeyPressAbstract.hpp"
#include "omoteconfig.h"

class Keys : public KeyPressAbstract {
public:
  Keys();
  Keys(QueueHandle_t queueHandle);
  void HandleKeyPresses() override;
  void QueueKeyEvent(KeyEvent aJustOccuredKeyEvent) override;

  static KeyId CharKeyToKeyId(char keyChar) {
    return charKeyToKeyIds.at(keyChar);
  }

protected:
  void GrabKeys();

private:
  static void KeyGrabberTask(void *aSelf);
  static void KeyProccessor(void *aSelf);

  QueueHandle_t mKeyPressQueueHandle;
  TaskHandle_t mKeyGrabbingTask;
  TaskHandle_t mKeyHandlingTask;

  // Keypad declarations
  static const byte ROWS = KEYPAD_ROWS; // 5;  // four rows
  static const byte COLS = KEYPAD_COLS; // 5;  // four columns
// define the symbols on the buttons of the keypads
#if not defined(OMOTE_HARDWARE_REV5)
  char hexaKeys[ROWS][COLS] = {
      {'s', '^', '-', 'm', 'r'}, //  source, channel+, Volume-,   mute, record
      {'i', 'R', '+', 'k', 'd'}, //    info,    right, Volume+,     OK,   down
      {'4', 'v', '1', '3', '2'}, //    blue, channel-,     red, yellow,  green
      {'>', 'o', 'b', 'u', 'L'}, // forward,      off,    back,     up,   left
      {'?', 'p', 'c', '<', '='}  //       ?,     play,  config, rewind,   stop
  };
#endif
  // Note: ? row/column entry is unused in hardware key matrix

  // TODO Should be able to optomize this out by reordering Ids at some point
  // or even using interrupts to trigger key press queueing
  static inline const std::map<char, KeyId> charKeyToKeyIds{
      {'o', KeyId::Power},
      // Top 4 Buttons left to right
      {'=', KeyId::Stop},
      {'<', KeyId::Rewind},
      {'p', KeyId::Play},
      {'>', KeyId::FastForward},
      // Buttons around D Pad
      {'c', KeyId::Menu},
      {'i', KeyId::Info},
      {'b', KeyId::Back},
      {'s', KeyId::Source},
      // D Pad
      {'u', KeyId::Up},
      {'d', KeyId::Down},
      {'L', KeyId::Left},
      {'R', KeyId::Right},
      {'k', KeyId::Center},
      // Volume Channel and 2 between
      {'+', KeyId::VolUp},
      {'-', KeyId::VolDown},
      {'m', KeyId::Mute},
      {'r', KeyId::Record},
      {'^', KeyId::ChannelUp},
      {'v', KeyId::ChannelDown},
      // Bottom 4 buttons left to right
      {'1', KeyId::Aux1},
      {'2', KeyId::Aux2},
      {'3', KeyId::Aux3},
      {'4', KeyId::Aux4},
      {'?', KeyId::INVALID}, // no physical key, should not happen
      // 3661 Extended keyboard codes
      {'g', KeyId::Guide},
      {'h', KeyId::Home},
      {'y', KeyId::Cycle},
      {'x', KeyId::Exit},
      {'P', KeyId::Pause},
      {'T', KeyId::TV},
      {'S', KeyId::Stream},
      {'B', KeyId::STB},
      {'A', KeyId::Audio},
      {'Y', KeyId::BluRay},
      {'D', KeyId::DVD}};

#if not defined(OMOTE_HARDWARE_REV5)
  byte rowPins[ROWS] = {SW_A, SW_B, SW_C, SW_D,
                        SW_E}; // connect to the row pinouts of the keypad
  byte colPins[COLS] = {SW_1, SW_2, SW_3, SW_4,
                        SW_5}; // connect to the column pinouts of the keypad
  Keypad customKeypad =
      Keypad(makeKeymap(hexaKeys), rowPins, colPins, ROWS, COLS);
#endif
};