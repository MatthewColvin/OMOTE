#pragma once

#include <LovyanGFX.hpp>
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

class LGFX : public lgfx::LGFX_Device {
 public:
  LGFX(void);

 private:
  #ifdef OMOTE_KEYBRD_3661
    //use modified values from panel datasheet rather than LGFX defaults
    //Not convinced it makes a huge difference but tweaks a few settings that could improve colour accuracy
    lgfx::Panel_ST7789_NHD _panel_instance;
  #else
    lgfx::Panel_ILI9341 _panel_instance;
  #endif
  #if defined (OMOTE_HARDWARE_REV5)
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
  #if defined (OMOTE_HARDWARE_REV5)
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
  virtual void setBrightness(uint8_t brightness) override;
  virtual uint8_t getBrightness() override;
  virtual void turnOff() override;
  virtual void setDayMode(bool isDay) override;
  virtual void getTouchData() override;

  std::shared_ptr<Notification<TouchPointType>> TouchNotification() {
    return mTouchEvent;
  }

  void wake();
  void sleep();

 protected:
  void flushDisplay(lv_disp_t *disp, const lv_area_t *area, uint8_t *pixelMap);

  void screenInput(lv_indev_t *indev, lv_indev_data_t *data);

  /// @brief Fade toward brightness based on isAwake
  /// @return True - Fade complete
  ///         False - Fade set point not reached
  bool fade();
  /// @brief Start the Fade task
  void startFade();

  /// @brief Set the actual display brightness right now
  /// @param brightness
  void setCurrentBrightness(uint8_t brightness);

 private:
  Display(int backlight_pin, int enable_pin);
  void setupTFT();
  void setupBacklight();

  int mEnablePin;
  int mBacklightPin;
  LGFX tft;

  uint8_t *bufA;
  uint8_t *bufB;
  //uint8_t bufA[DRAW_BUF_SIZE];
  //uint8_t bufB[DRAW_BUF_SIZE];

  TouchPointType mTouchPoint;
  TouchPointType mOldPoint;
  std::shared_ptr<Notification<TouchPointType>> mTouchEvent =
      std::make_shared<Notification<TouchPointType>>();

  TaskHandle_t mDisplayFadeTask = nullptr;
  SemaphoreHandle_t mFadeTaskMutex = nullptr;
  static void fadeImpl(void *aBrightness);

  uint8_t mBrightness = 0;       // Current display brightness
  uint8_t mAwakeBrightness = 0;  // Current setting for brightness when awake
  bool mIsAsleep = false;
  bool mIsDay = true;

  bool mHaveTouch = false;
  uint16_t mTouchX = 0;
  uint16_t mTouchY = 0;
};
