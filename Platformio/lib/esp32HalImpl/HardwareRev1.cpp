#include "HardwareRev1.hpp"

#include "omoteconfig.h"

void HardwareRev1::init() {
#if not defined(OMOTE_HARDWARE_REV5)
  mKeys = std::make_shared<Keys>();
  mBattery = std::make_shared<Battery>(ADC_BAT, CRG_STAT);
#endif
}

void HardwareRev1::initIO() {
  HardwareRevX::initIO();
#if not defined(OMOTE_HARDWARE_REV5)
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
#endif

#if not defined(OMOTE_HARDWARE_REV5)
  pinMode(ADC_BAT, INPUT);
#endif

#if not defined(OMOTE_HARDWARE_REV5)
  gpio_hold_dis((gpio_num_t)SW_1);
  gpio_hold_dis((gpio_num_t)SW_2);
  gpio_hold_dis((gpio_num_t)SW_3);
  gpio_hold_dis((gpio_num_t)SW_4);
  gpio_hold_dis((gpio_num_t)SW_5);
#endif
}

void HardwareRev1::sleepDisplayPins() {
#if not defined(OMOTE_HARDWARE_REV5)
  digitalWrite(LCD_MOSI, LOW);
  digitalWrite(LCD_SCK, LOW);
#endif
}

void HardwareRev1::configPinsForSleepInterrupts() {
#if not defined(OMOTE_HARDWARE_REV5)
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
#endif
};