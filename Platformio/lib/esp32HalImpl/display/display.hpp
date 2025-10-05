#pragma once

#include <LovyanGFX.hpp>
#include <atomic>
#include <memory>

#include "Hardware/DisplayAbstract.h"
#include "HardwareAbstract.hpp"
#include "Notification.hpp"
#include "driver/ledc.h"
#ifdef OMOTE_KEYBRD_3661
#include "Panel_ST7789_NHD.h"
#include "Touch_FT5x26.h"
#endif

/*LEDC Channel to use for the LCD backlight*/
#define LCD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_5

#define LCD_BACKLIGHT_LEDC_FREQUENCY 640

#define LCD_BACKLIGHT_LEDC_BIT_RESOLUTION 8

#define DEFAULT_BACKLIGHT_BRIGHTNESS 128

namespace lvgx = lgfx::v1;
/*LEDC Channel to use for the keypad backlight*/
#define KBD_BACKLIGHT_LEDC_CHANNEL LEDC_CHANNEL_6

class LGFX : public lgfx::LGFX_Device {
public:
  LGFX(void);

private:
#ifdef OMOTE_KEYBRD_3661
  // use modified values from panel datasheet rather than LGFX defaults
  // Not convinced it makes a huge difference but tweaks a few settings that could improve colour accuracy
  lgfx::Panel_ST7789_NHD _panel_instance;
#else
  lgfx::Panel_ILI9341 _panel_instance;
#endif
#if defined(OMOTE_HARDWARE_REV5)
  lgfx::Bus_Parallel8 _bus_instance;
#else
  lgfx::Bus_SPI _bus_instance;
#endif
#ifdef OMOTE_KEYBRD_3661
  lgfx::Touch_FT5x26 _touch_instance;
#else
  lgfx::Touch_FT5x06 _touch_instance;
#endif
};

class Display : public DisplayAbstract {
public:
  typedef struct {
    float delta;
    float startBrightness;
    uint8_t targetBrightness;
    uint16_t delay;
  } thread_args;

#if defined(OMOTE_HARDWARE_REV5)
  // have PSRAM so use full screen buffers
  static constexpr auto DRAW_BUF_SIZE =
      SCREEN_WIDTH * SCREEN_HEIGHT * (LV_COLOR_DEPTH / 8);
#else
  static constexpr auto DRAW_BUF_SIZE =
      SCREEN_WIDTH * SCREEN_HEIGHT / 10 * (LV_COLOR_DEPTH / 8);
#endif

  using TouchPointType = std::pair<int, int>;

  static std::shared_ptr<Display> getInstance();

  /// @brief Set brightness setting and fade to it
  /// @param brightness

  virtual void setLcdDayBrightness(uint8_t brightness, bool instant = false) override;
  virtual void setLcdNightBrightness(uint8_t brightness, bool instant = false) override;
  virtual void setKbdDayBrightness(uint8_t brightness, bool instant = false) override;
  virtual void setKbdNightBrightness(uint8_t brightness, bool instant = false) override;
  virtual uint8_t getLcdDayBrightness() override;
  virtual uint8_t getLcdNightBrightness() override;
  virtual uint8_t getKbdDayBrightness() override;
  virtual uint8_t getKbdNightBrightness() override;
  virtual void initBrightnessLevels (uint8_t lcdDay, uint8_t lcdNight, uint8_t kbdDay, uint8_t kbdNight) override 
      {mLcdDayBrightness = lcdDay; mLcdNightBrightness = lcdNight; mKbdDayBrightness = kbdDay; mKbdNightBrightness = kbdNight;};
  virtual void startFade(uint16_t delay) override {startLcdFade(false, delay); startKbdFade(false, delay);};

  virtual void turnOff() override;
  virtual void setDayMode(bool isDay) override;
  virtual void getTouchData() override;

  std::shared_ptr<Notification<TouchPointType>> TouchNotification() {
    return mTouchEvent;
  }

  void wake();
  void sleep();

  void reInit();

protected:
  void flushDisplay(lv_disp_t *disp, const lv_area_t *area, uint8_t *pixelMap);

  void screenInput(lv_indev_t *indev, lv_indev_data_t *data);

  /// @brief Fade toward brightness based on isAwake
  /// @param instant - instant transition if true, gradual transition id false
  /// @param delay - delay till start of transition
  /// @brief Start the Fade task
  void startLcdFade(bool instant = false, uint16_t delay = 0);
  void startKbdFade(bool instant = false, uint16_t delay = 0);

  /// @brief Set the actual display brightness right now
  /// @param brightness
  void setCurrentLcdBrightness(uint8_t brightness);
  void setCurrentKbdBrightness(uint8_t brightness);

private:
  Display(int backlight_pin, int enable_pin);
  void setupTFT();
  void setupBacklight();

  int mEnablePin;
  int mBacklightPin;
  LGFX tft;

  uint8_t *bufA;
  uint8_t *bufB;
  // uint8_t bufA[DRAW_BUF_SIZE];
  // uint8_t bufB[DRAW_BUF_SIZE];

  TouchPointType mTouchPoint;
  TouchPointType mOldPoint;
  std::shared_ptr<Notification<TouchPointType>> mTouchEvent =
      std::make_shared<Notification<TouchPointType>>();

  TaskHandle_t mDisplayLcdFadeTask = nullptr;
  SemaphoreHandle_t mFadeLcdTaskMutex = nullptr;
  TaskHandle_t mDisplayKbdFadeTask = nullptr;
  SemaphoreHandle_t mFadeKbdTaskMutex = nullptr;
  static void fadeLcdImpl(void *aBrightness);
  static void fadeKbdImpl(void *aBrightness);

  // not all used for all hardware but simplest to have them all here than #defs
  // atomic not needed as 8-bit types but included for clarity
  std::atomic<uint8_t> mLcdBrightness = 0; // Current display brightness
  std::atomic<uint8_t> mKbdBrightness = 0; // Current keyboard brightness
  uint8_t mLcdDayBrightness = 0;           // Current setting for brightness when day mode
  uint8_t mKbdDayBrightness = 0;           // Current keyboard for brightness when day mode
  uint8_t mLcdNightBrightness = 0;         // Current display for brightness when night mode
  uint8_t mKbdNightBrightness = 0;         // Current keyboard for brightness when night mode

  uint8_t mIsAsleep = false;
  uint8_t mIsDay = true;

  bool mHaveTouch = false;
  uint16_t mTouchX = 0;
  uint16_t mTouchY = 0;

  thread_args mLcdArgs = {0, 0, 0, 0};
  thread_args mKbdArgs = {0, 0, 0, 0};
};
