#pragma once
//#include <Keypad.h> // modified for inverted logic

#include <map>

#include "Hardware/KeyPressAbstract.hpp"
//#include "omoteconfig.h"

class Keys : public KeyPressAbstract {
public:
  Keys();

  void HandleKeyPresses(const KeyEvent &aJustOccuredKeyEvent) override;

  static KeyId CharKeyToKeyId(char keyChar) {
    return charKeyToKeyIds.at(keyChar);
  };

  static bool isValidId(char keyChar) {
    return (charKeyToKeyIds.count(keyChar) > 0);
  };

private:
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
};