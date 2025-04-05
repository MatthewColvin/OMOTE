#pragma once
#include "WidgetBase.hpp"
#include "lvgl.h"

namespace UI::Widget {
class Image : public Base {
public:
  /**
   * Construct Image Given a Symbol
   *  EX: Image(LV_SYMBOL_AUDIO);
   */
  Image(const char *aSymbol);
  virtual ~Image() = default;

  void SetRotation(int16_t aAngle);
  void SetZoom(uint16_t aZoom);
  void SetAntialias(bool aEnable);
  void SetOffset(lv_coord_t aX, lv_coord_t aY);
};

} // namespace UI::Widget
