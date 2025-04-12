#include "HardwareRev1.hpp"

#include "Rev1LittleFs.hpp"
#include "Rev1PinDefs.h"
#include "omoteconfig.h"

void HardwareRev1::init() {
  mLittleFs = Rev1LittleFs::getInstance();
  HardwareRevX::init();
  mKeys = std::make_shared<Keys>();
  mLittleFs->mount();
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
  digitalWrite(LCD_MOSI, LOW);
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