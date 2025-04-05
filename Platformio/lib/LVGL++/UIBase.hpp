// OMOTE UI
// 2023 Matthew Colvin

#pragma once
#include "HardwareAbstract.hpp"
#include <memory>

namespace UI {

class UIBase {
public:
  UIBase();

  virtual void loopHandler();

  virtual void restore() = 0;

protected:
};

} // namespace UI