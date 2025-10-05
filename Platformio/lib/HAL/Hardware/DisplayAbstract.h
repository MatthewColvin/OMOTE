#pragma once
#include <memory>

#include "lvgl.h"
class DisplayAbstract {
public:
  virtual ~DisplayAbstract() = default;

  virtual void setLcdDayBrightness(uint8_t brightness, bool instant = false) = 0;
  virtual void setKbdDayBrightness(uint8_t brightness, bool instant = false) = 0;
  virtual void setLcdNightBrightness(uint8_t brightness, bool instant = false) = 0;
  virtual void setKbdNightBrightness(uint8_t brightness, bool instant = false) = 0;
  virtual uint8_t getLcdDayBrightness() = 0;
  virtual uint8_t getLcdNightBrightness() = 0;
  virtual uint8_t getKbdDayBrightness() = 0;
  virtual uint8_t getKbdNightBrightness() = 0;
  virtual void initBrightnessLevels(uint8_t lcdDay, uint8_t lcdNight, uint8_t kbdDay, uint8_t kbdNight) = 0;
  // May not actually need to be apart of interface
  virtual void startFade(uint16_t delay) = 0;

  virtual void turnOff() = 0;
  virtual void setDayMode(bool isDay) = 0;
  virtual void getTouchData() = 0;

  void ForceRefresh();

protected:
  DisplayAbstract() = default;
  // Set this in the constructor of the Child Calss
  lv_display_t *mDisplay{nullptr};
  // Set this with a getInstance method in the Child Class
  static inline std::shared_ptr<DisplayAbstract> mInstance;
};

inline void DisplayAbstract::ForceRefresh() { lv_refr_now(mDisplay); }