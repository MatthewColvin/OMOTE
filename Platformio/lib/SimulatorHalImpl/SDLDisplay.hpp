#pragma once
#include <stdint.h>

#include "Hardware/DisplayAbstract.h"
#include "SDL2/SDL.h"

class SDLDisplay : public DisplayAbstract {
public:
  static std::shared_ptr<SDLDisplay> getInstance();

  virtual void setLcdDayBrightness(uint8_t brightness, bool instant = false) override { mLcdDayBrightness = brightness; };
  virtual void setLcdNightBrightness(uint8_t brightness, bool instant = false) override { mLcdNightBrightness = brightness; };
  virtual void setKbdDayBrightness(uint8_t brightness, bool instant = false) override { mKbdDayBrightness = brightness; };
  virtual void setKbdNightBrightness(uint8_t brightness, bool instant = false) override { mKbdNightBrightness = brightness; };
  virtual uint8_t getLcdDayBrightness() override { return mLcdDayBrightness; };
  virtual uint8_t getLcdNightBrightness() override { return mLcdNightBrightness; };
  virtual uint8_t getKbdDayBrightness() override { return mKbdDayBrightness; };
  virtual uint8_t getKbdNightBrightness() override { return mKbdNightBrightness; };

  virtual void initBrightnessLevels(uint8_t lcdDay, uint8_t lcdNight, uint8_t kbdDay, uint8_t kbdNight) override {
    mLcdDayBrightness = lcdDay;
    mLcdNightBrightness = lcdNight;
    mKbdDayBrightness = kbdDay;
    mKbdNightBrightness = kbdNight;
  };
  virtual void startFade(uint16_t delay) override {};

  virtual void turnOff() override;
  virtual void setDayMode(bool isDay) override;
  virtual void getTouchData() override;

  void setTitle(std::string aNewTitle);

protected:
private:
  SDLDisplay();
  uint8_t mLcdDayBrightness = 0;   // Current setting for brightness when day mode
  uint8_t mKbdDayBrightness = 0;   // Current keyboard for brightness when day mode
  uint8_t mLcdNightBrightness = 0; // Current display for brightness when night mode
  uint8_t mKbdNightBrightness = 0; // Current keyboard for brightness when night mode
  SDL_Window *mSimWindow;

  bool mIsDayModeActive = false;
};