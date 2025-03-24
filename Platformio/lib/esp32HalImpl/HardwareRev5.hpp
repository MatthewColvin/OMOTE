#pragma once

#include "HardwareRev1.hpp"

class HardwareRev5 : public HardwareRev1 {
 public:
  HardwareRev5() = default;
  virtual ~HardwareRev5() = default;

 protected:
  void init() override;

 private:
  void setupKeyboard();

  void keyboardScan() override;

  // keypad scanning
  Adafruit_TCA8418 keypad;
};
