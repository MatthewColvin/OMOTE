#pragma once
#include "HardwareFactory.hpp"
#include "HomeAssistUI.hpp"
#include "JsonUI.hpp"

namespace OMOTE {
std::shared_ptr<UI::UIBase> ui = nullptr;

void createUI() {
  // ui = std::make_unique<UI::BasicUI>();
  ui = std::make_unique<UI::JsonUI>();
  // ui = std::make_unique<UI::HomeAssistUI>();
  ui->restore();
}

void setup() {
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
