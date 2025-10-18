#include "HardwareRev1.hpp"

#include "Rev1PinDefs.h"
#include "omoteconfig.h"
#include <LittleFS.h>

void HardwareRev1::init() {
  Serial.begin(115200);
  LittleFS.begin(true);
  LoggingInterface::restoreSettings();
  HardwareRevX::init();
  mBattery = std::make_shared<BatteryRev1>(ADC_BAT, CRG_STAT);
  mKeys = std::make_shared<Keys>();
}

void HardwareRev1::initIO() {
  HardwareRevX::initIO();
  pinMode(SW_1, OUTPUT);
  pinMode(SW_2, OUTPUT);
  pinMode(SW_3, OUTPUT);
  pinMode(SW_4, OUTPUT);
  pinMode(SW_5, OUTPUT);
  pinMode(SW_A, INPUT);
  pinMode(SW_B, INPUT);
  pinMode(SW_C, INPUT);
  pinMode(SW_D, INPUT);
  pinMode(SW_E, INPUT);

  pinMode(ADC_BAT, INPUT);

  gpio_hold_dis((gpio_num_t)SW_1);
  gpio_hold_dis((gpio_num_t)SW_2);
  gpio_hold_dis((gpio_num_t)SW_3);
  gpio_hold_dis((gpio_num_t)SW_4);
  gpio_hold_dis((gpio_num_t)SW_5);
}

void HardwareRev1::sleepDisplayPins() {
  pinMode(LCD_MOSI, OUTPUT);
  digitalWrite(LCD_MOSI, LOW);
  pinMode(LCD_SCK, OUTPUT);
  digitalWrite(LCD_SCK, LOW);
}

void HardwareRev1::configPinsForSleepInterrupts() {
  pinMode(SW_1, OUTPUT);
  pinMode(SW_2, OUTPUT);
  pinMode(SW_3, OUTPUT);
  pinMode(SW_4, OUTPUT);
  pinMode(SW_5, OUTPUT);
  digitalWrite(SW_1, HIGH);
  digitalWrite(SW_2, HIGH);
  digitalWrite(SW_3, HIGH);
  digitalWrite(SW_4, HIGH);
  digitalWrite(SW_5, HIGH);
  gpio_hold_en((gpio_num_t)SW_1);
  gpio_hold_en((gpio_num_t)SW_2);
  gpio_hold_en((gpio_num_t)SW_3);
  gpio_hold_en((gpio_num_t)SW_4);
  gpio_hold_en((gpio_num_t)SW_5);
};

bool HardwareRev1::keyboardScan() {
  bool retVal = false;
  if (!customKeypad.getKeys()) {
    return false; // no activity return early.
  }
  for (int i = 0; i < LIST_MAX; i++) {
    if (customKeypad.key[i].kstate == PRESSED ||
        customKeypad.key[i].kstate == RELEASED) {
      auto eventType = customKeypad.key[i].kstate == PRESSED
                           ? KeyPressAbstract::KeyEvent::Type::Press
                           : KeyPressAbstract::KeyEvent::Type::Release;
      const auto keyChar = customKeypad.key[i].kchar;
      auto stateChange = customKeypad.key[i].stateChanged;
      if (Keys::isValidId && stateChange) {
        mKeys->HandleKeyPresses(KeyPressAbstract::KeyEvent(Keys::CharKeyToKeyId(keyChar), eventType));
        if (eventType == KeyPressAbstract::KeyEvent::Type::Press)
          retVal = true;
      }
    }
  }
  return retVal;
}
