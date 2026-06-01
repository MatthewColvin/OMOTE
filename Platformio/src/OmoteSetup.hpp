#pragma once
#include "HardwareFactory.hpp"
#include "HomeAssistUI.hpp"
#include "JsonUI.hpp"
#ifdef OMOTE_HARDWARE_REV5
#include "Arduino.h"
#endif

namespace OMOTE {
std::shared_ptr<UI::UIBase> ui = nullptr;

void createUI() {
  // ui = std::make_unique<UI::BasicUI>();
  ui = std::make_unique<UI::JsonUI>();
  //ui = std::make_unique<UI::HomeAssistUI>();
  ui->restore();
}

void setup() {
#ifdef OMOTE_HARDWARE_REV5
  // Safety check to make sure hardware matches software build and protect IR LED
  // GPIO3 is not connected on Rev5 and pulled up on Rev5_3661
  // If level is wrong go into deep sleep
  pinMode(3, INPUT_PULLDOWN);
  delay(5);
#ifdef OMOTE_KEYBRD_3661
  if (LOW == digitalRead(3)) {
    pinMode(3, INPUT); // prevent increased current due to external pull up resistor
    esp_deep_sleep_start();
  }
  pinMode(3, INPUT);
#else
  if (HIGH == digitalRead(3))
    esp_deep_sleep_start();
#endif
#endif

  lv_init();

  HardwareFactory::Init();
  HardwareFactory::getAbstract().wifi()->begin();

  createUI();
  lv_timer_handler(); // Run the LVGL UI once before the loop takes over
}

void loop() {
  HardwareFactory::getAbstract().loopHandler();
  ui->loopHandler();
}

} // namespace OMOTE
