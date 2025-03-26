
#include "display.hpp"

#include "driver/ledc.h"
#include "omoteconfig.h"

LGFX::LGFX(void) {
  {
    auto cfg = _bus_instance.config();
#if defined(OMOTE_HARDWARE_REV5)
#ifdef OMOTE_KEYBRD_3661
    cfg.freq_write = 20000000;
#else
    cfg.freq_write = SPI_FREQUENCY;
#endif
    cfg.pin_wr = LCD_WR;
    cfg.pin_rd = LCD_RD;
    cfg.pin_rs = LCD_DC;
    cfg.pin_d0 = LCD_D0;
    cfg.pin_d1 = LCD_D1;
    cfg.pin_d2 = LCD_D2;
    cfg.pin_d3 = LCD_D3;
    cfg.pin_d4 = LCD_D4;
    cfg.pin_d5 = LCD_D5;
    cfg.pin_d6 = LCD_D6;
    cfg.pin_d7 = LCD_D7;
#else
    cfg.freq_write = SPI_FREQUENCY;
    cfg.freq_read = 16000000;
    cfg.dma_channel = SPI_DMA_CH_AUTO;
    cfg.pin_sclk = LCD_SCK;
    cfg.pin_mosi = LCD_MOSI;
    cfg.pin_dc = LCD_DC;
#endif
    _bus_instance.config(cfg);
    _panel_instance.setBus(&_bus_instance);
  }
  {
    auto cfg = _panel_instance.config();
    cfg.pin_cs = LCD_CS;
    cfg.pin_rst = -1;
    cfg.pin_busy = -1;
    cfg.memory_width = SCREEN_WIDTH;
    cfg.memory_height = SCREEN_HEIGHT;
    cfg.offset_x         =     0;  
    cfg.offset_y         =     0;  
    cfg.dummy_read_pixel =     8;  
    cfg.dummy_read_bits  =     1;  
    cfg.readable         =  true;  
    cfg.invert           = true;  
    cfg.rgb_order        = false;  
    cfg.dlen_16bit       = false;  
    cfg.bus_shared       =  true; 
    cfg.panel_width = SCREEN_WIDTH;
    cfg.panel_height = SCREEN_HEIGHT;
    #ifdef OMOTE_KEYBRD_3661
    cfg.invert           = true;
    #endif
    cfg.offset_rotation = 2;
    _panel_instance.config(cfg);
  }
  {
    auto cfg = _touch_instance.config();
    cfg.i2c_addr = 0x38;
    cfg.i2c_port = 0;
    cfg.pin_sda = SDA;
    cfg.pin_scl = SCL;
    cfg.freq = 400000;
    cfg.x_min = 0;
    cfg.x_max = SCREEN_WIDTH - 1;
    cfg.y_min = 0;
    cfg.y_max = SCREEN_HEIGHT - 1;
    _touch_instance.config(cfg);
    _panel_instance.setTouch(&_touch_instance);
  }
  setPanel(&_panel_instance);
}

std::shared_ptr<Display> Display::getInstance() {
  if (DisplayAbstract::mInstance == nullptr) {
    DisplayAbstract::mInstance =
        std::shared_ptr<Display>(new Display(LCD_BL, LCD_EN));
  }
  return std::static_pointer_cast<Display>(mInstance);
}

Display::Display(int backlight_pin, int enable_pin)
    : DisplayAbstract(), mBacklightPin(backlight_pin), mEnablePin(enable_pin) {

  bufA = (uint8_t *)malloc(DRAW_BUF_SIZE);
  bufB = (uint8_t *)malloc(DRAW_BUF_SIZE);

  mDisplay = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(mDisplay, [](auto aDisplay, auto aArea, auto aPxMap) {
    getInstance()->flushDisplay(aDisplay, aArea, aPxMap);
  });

  #if defined (OMOTE_HARDWARE_REV5)
  lv_display_set_buffers(mDisplay, bufA, bufB, DRAW_BUF_SIZE,
                         LV_DISPLAY_RENDER_MODE_FULL);
  #else
  lv_display_set_buffers(mDisplay, bufA, bufB, DRAW_BUF_SIZE,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
  #endif

  Serial.println("Display buffers set");

  lv_tick_set_cb([] { return static_cast<uint32_t>(millis()); });

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, [](auto aIndev, auto aData) {
    getInstance()->screenInput(aIndev, aData);
  });

  pinMode(mEnablePin, OUTPUT);
  digitalWrite(mEnablePin, HIGH);
  pinMode(mBacklightPin, OUTPUT);
#if defined(OMOTE_KEYBRD_3661)
  digitalWrite(mBacklightPin, LOW);
#else
  digitalWrite(mBacklightPin, HIGH);
#endif

  setupBacklight();  // This eliminates the flash of the backlight

#if not defined(OMOTE_HARDWARE_REV5)
  // Slowly charge the VSW voltage to prevent a brownout
  // Workaround for hardware rev 1!
  for (int i = 0; i < 100; i++) {
    digitalWrite(this->mEnablePin, HIGH);  // LCD Logic off
    delayMicroseconds(1);
    digitalWrite(this->mEnablePin, LOW);  // LCD Logic on
  }
#else
  digitalWrite(this->mEnablePin, LOW);  // LCD Logic on
#endif

  setupTFT();

  mFadeTaskMutex = xSemaphoreCreateBinary();
  xSemaphoreGive(mFadeTaskMutex);
}

void Display::wake() {
  if (mIsAsleep) {
    mIsAsleep = false;
    startFade();
  }
}

void Display::sleep() {
  if (!mIsAsleep) {
    mIsAsleep = true;
    startFade();
  }
}

void Display::setDayMode(bool isDay) {
  if (isDay != mIsDay) {
    mIsDay = isDay;
    startFade();
  }
}

void Display::setupBacklight() {
  // Configure the backlight PWM
  // Manual setup because ledcSetup() briefly turns on the backlight
  ledc_channel_config_t ledc_channel_left;
  ledc_channel_left.gpio_num = (gpio_num_t)mBacklightPin;
  ledc_channel_left.speed_mode = LEDC_SPEED_MODE;
  ledc_channel_left.channel = LEDC_CHANNEL_5;
  ledc_channel_left.intr_type = LEDC_INTR_DISABLE;
  ledc_channel_left.timer_sel = LEDC_TIMER_1;
#ifdef OMOTE_KEYBRD_3661
  ledc_channel_left.flags.output_invert = 0;
#else
  ledc_channel_left.flags.output_invert = 1;  // Can't do this with ledcSetup()
#endif
  ledc_channel_left.duty = 0;
  ledc_channel_left.hpoint = 0;
  ledc_timer_config_t ledc_timer;
  ledc_timer.speed_mode = LEDC_SPEED_MODE;
  ledc_timer.duty_resolution = LEDC_TIMER_8_BIT;
  ledc_timer.timer_num = LEDC_TIMER_1;
  ledc_timer.clk_cfg = LEDC_USE_APB_CLK;
  ledc_timer.freq_hz = 640;
  ledc_channel_config(&ledc_channel_left);
  ledc_timer_config(&ledc_timer);
}

void Display::setupTFT() {
  delay(100);
  Serial.println("Initialising TFT:");
  tft.init();
  tft.initDMA();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
}

void Display::setBrightness(uint8_t brightness) {
  mAwakeBrightness = brightness;
  Serial.print("Set Brightness:");
  Serial.println(mAwakeBrightness);
  startFade();
}

uint8_t Display::getBrightness() { return mAwakeBrightness; }

void Display::setCurrentBrightness(uint8_t brightness) {
  mBrightness = brightness;
  auto duty = static_cast<int>(mBrightness);
  ledcWrite(LCD_BACKLIGHT_LEDC_CHANNEL, duty);
  // Serial.print("Current Brightness:");
  // Serial.println(mBrightness);
}

void Display::turnOff() {
#if defined(OMOTE_KEYBRD_3661)
  digitalWrite(this->mBacklightPin, LOW);
#else
  digitalWrite(this->mBacklightPin, HIGH);
#endif
  digitalWrite(this->mEnablePin, HIGH);
  pinMode(this->mBacklightPin, INPUT);
  pinMode(this->mEnablePin, INPUT);
  gpio_hold_en((gpio_num_t)mBacklightPin);
  gpio_hold_en((gpio_num_t)mEnablePin);
}

void Display::getTouchData() {
  mHaveTouch = tft.getTouch(&mTouchX, &mTouchY);
}

void Display::screenInput(lv_indev_t *indev, lv_indev_data_t *data) {
  if (mHaveTouch) {
    mHaveTouch = false;
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = mTouchX;
    data->point.y = mTouchY;
    mTouchPoint = {mTouchX, mTouchY};
    mTouchEvent->notify(mTouchPoint);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void Display::fadeImpl(void *) {
  bool fadeDone = false;
  while (!fadeDone) {
    fadeDone = getInstance()->fade();
    vTaskDelay(3 / portTICK_PERIOD_MS);  // 3 miliseconds between steps
    // 0 - 255 will take about .75 seconds to fade up.
  }

  xSemaphoreTake(getInstance()->mFadeTaskMutex, portMAX_DELAY);
  getInstance()->mDisplayFadeTask = nullptr;
  xSemaphoreGive(getInstance()->mFadeTaskMutex);

  vTaskDelete(nullptr);  // Delete Fade Task
}

bool Display::fade() {
  // Early return no fade needed.
  uint8_t targetBrightness;
  if(mIsDay)
    targetBrightness = mAwakeBrightness;
  else
    targetBrightness = mAwakeBrightness/4;

  if (mBrightness == targetBrightness || mIsAsleep && mBrightness == 0) {
    return true;
  }

  bool fadeDown = mIsAsleep || mBrightness > targetBrightness;
  if (fadeDown) {
    setCurrentBrightness(mBrightness - 1);
    auto setPoint = mIsAsleep ? 0 : targetBrightness;
    return mBrightness == setPoint;
  } else {
    setCurrentBrightness(mBrightness + 1);
    return mBrightness == targetBrightness;
  }
}

void Display::startFade() {
  xSemaphoreTake(mFadeTaskMutex, portMAX_DELAY);
  // Only Create Task if it is needed
  if (mDisplayFadeTask == nullptr) {
    xTaskCreate(&Display::fadeImpl, "Display Fade Task", 1024, nullptr, 5,
                &mDisplayFadeTask);
  }
  xSemaphoreGive(mFadeTaskMutex);
}

void Display::flushDisplay(lv_disp_t *disp, const lv_area_t *area,
                           uint8_t *pixelMap) {
  uint32_t w = (area->x2 - area->x1 + 1);
  uint32_t h = (area->y2 - area->y1 + 1);

  /*uint16_t *buf = (uint16_t *)pixelMap;
  for(uint16_t i=0;i<(w*h);i++) {
    buf[i] = 0x0010; //force to all red
  }
  Serial.printf("Flushing %d words\r\n",(w*h));

  for(uint16_t i=0;i<(w*h);i++) {
    if(buf[i] != 0x0010)
      Serial.printf("E:%d\r\n",i);
  }*/
  

  tft.startWrite();
  tft.setAddrWindow(area->x1, area->y1, w, h);
  //tft.writePixelsDMA((uint16_t *)pixelMap, w * h, true);
  tft.pushPixelsDMA((uint16_t *)pixelMap, w * h);
  tft.endWrite();

  lv_display_flush_ready(disp);
}
