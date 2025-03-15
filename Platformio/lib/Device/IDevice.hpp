#pragma once

#include <memory>
#include <string>

#include "Hardware/KeyPressAbstract.hpp"
#include "PageBase.hpp"
#include "lvgl.h"

class IDevice {
 public:
  using Ptr = std::shared_ptr<IDevice>;
  virtual ~IDevice() = default;

  // Core device information
  virtual std::string GetName() const = 0;
  virtual lv_color_t GetDisplayColor() const = 0;
  virtual bool IsDeleteAble() const = 0;

  // Interaction handlers
  virtual bool HandleKeyEvent(KeyPressAbstract::KeyEvent event) = 0;
  virtual std::unique_ptr<UI::Page::Base> GetControlPage() = 0;
};
