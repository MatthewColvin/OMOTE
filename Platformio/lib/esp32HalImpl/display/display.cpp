
#include "display.hpp"

#include "driver/ledc.h"
#include "omoteconfig.h"

LGFX::LGFX(void) {
  {
    auto cfg = _bus_instance.config();
#if defined(OMOTE_HARDWARE_REV5)
    cfg.freq_write = SPI_FREQUENCY;
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
    cfg.offset_x = 0;
    cfg.offset_y = 0;
    cfg.dummy_read_pixel = 8;
    cfg.dummy_read_bits = 1;
    cfg.readable = true;
    cfg.invert = false;
    cfg.rgb_order = false;
    cfg.dlen_16bit = false;
    cfg.bus_shared = true;
    cfg.panel_width = SCREEN_WIDTH;
    cfg.panel_height = SCREEN_HEIGHT;
#ifdef OMOTE_KEYBRD_3661
    cfg.invert = true;
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

  pinMode(mEnablePin, OUTPUT);
  digitalWrite(mEnablePin, HIGH);
#if defined(OMOTE_KEYBRD_3661)
  digitalWrite(mBacklightPin, LOW);
#else
  digitalWrite(mBacklightPin, HIGH);
#endif
  pinMode(mBacklightPin, OUTPUT);

  bufA = (uint8_t *)malloc(DRAW_BUF_SIZE);
  bufB = (uint8_t *)malloc(DRAW_BUF_SIZE);

  mDisplay = lv_display_create(SCREEN_WIDTH, SCREEN_HEIGHT);
  lv_display_set_flush_cb(mDisplay, [](auto aDisplay, auto aArea, auto aPxMap) {
    getInstance()->flushDisplay(aDisplay, aArea, aPxMap);
  });

#if defined(OMOTE_HARDWARE_REV5)
  lv_display_set_buffers(mDisplay, bufA, bufB, DRAW_BUF_SIZE,
                         LV_DISPLAY_RENDER_MODE_FULL);
#else
  lv_display_set_buffers(mDisplay, bufA, bufB, DRAW_BUF_SIZE,
                         LV_DISPLAY_RENDER_MODE_PARTIAL);
#endif

  // Serial.println("Display buffers set");

  lv_tick_set_cb([] { return static_cast<uint32_t>(millis()); });

  lv_indev_t *indev = lv_indev_create();
  lv_indev_set_type(indev, LV_INDEV_TYPE_POINTER);
  lv_indev_set_read_cb(indev, [](auto aIndev, auto aData) {
    getInstance()->screenInput(aIndev, aData);
  });

  setupBacklight();

#if not defined(OMOTE_HARDWARE_REV5)
  // Slowly charge the VSW voltage to prevent a brownout
  // Workaround for hardware rev 1!
  for (int i = 0; i < 100; i++) {
    digitalWrite(this->mEnablePin, HIGH); // LCD Logic off
    delayMicroseconds(1);
    digitalWrite(this->mEnablePin, LOW); // LCD Logic on
  }
#else
  digitalWrite(this->mEnablePin, LOW); // LCD Logic on
#endif

  setupTFT();

  mFadeLcdTaskMutex = xSemaphoreCreateBinary();
  xSemaphoreGive(mFadeLcdTaskMutex);

  mFadeKbdTaskMutex = xSemaphoreCreateBinary();
  xSemaphoreGive(mFadeKbdTaskMutex);
}

void Display::reInit() {

  mIsAsleep = false;
  mLcdBrightness = 0;
  mKbdBrightness = 0;

  pinMode(mEnablePin, OUTPUT);
  digitalWrite(mEnablePin, HIGH);
#if defined(OMOTE_KEYBRD_3661)
  digitalWrite(mBacklightPin, LOW);
#else
  digitalWrite(mBacklightPin, HIGH);
#endif
  pinMode(mBacklightPin, OUTPUT);

  setupBacklight();

#if not defined(OMOTE_HARDWARE_REV5)
  // Slowly charge the VSW voltage to prevent a brownout
  // Workaround for hardware rev 1!
  for (int i = 0; i < 100; i++) {
    digitalWrite(this->mEnablePin, HIGH); // LCD Logic off
    delayMicroseconds(1);
    digitalWrite(this->mEnablePin, LOW); // LCD Logic on
  }
#else
  digitalWrite(this->mEnablePin, LOW); // LCD Logic on
#endif

  tft.init();
  lv_obj_invalidate(lv_scr_act());

  startFade(50); // allow time for LCD init to complete before bringing up backlight
}

void Display::wake() {
  if (mIsAsleep) {
    mIsAsleep = false;
    startLcdFade();
    startKbdFade();
  }
}

void Display::sleep() {
  if (!mIsAsleep) {
    mIsAsleep = true;
    startLcdFade();
    startKbdFade();
  }
}

void Display::setDayMode(bool isDay) {
  //done this way so doesn't get stuck if isDay changes faster than fade can complete
  bool update = isDay ? mLcdBrightness != mLcdDayBrightness : mLcdBrightness != mLcdNightBrightness;
  if (update) {
    mIsDay = isDay;
    startLcdFade();
    startKbdFade();
  }
}

void Display::setupBacklight() {
  ledcSetClockSource(LEDC_USE_APB_CLK); // set to APB as default of XCLK doesn't seem to work

  // Configure the backlight PWM
  ledcAttachChannel((gpio_num_t)mBacklightPin, 640, LEDC_TIMER_8_BIT, LCD_BACKLIGHT_LEDC_CHANNEL);
#ifndef OMOTE_KEYBRD_3661
  ledcOutputInvert((gpio_num_t)mBacklightPin, true);
#endif
  ledcWrite((gpio_num_t)mBacklightPin, 0);

#ifdef OMOTE_HARDWARE_REV5
  // keyboard
  ledcAttachChannel(KBD_BL, 5000, LEDC_TIMER_8_BIT, KBD_BACKLIGHT_LEDC_CHANNEL);
  ledcWrite(KBD_BL, 0);
#endif
}

void Display::setupTFT() {
  delay(100);
  // Serial.println("Initialising TFT:");
  tft.init();
  tft.initDMA();
  tft.setRotation(0);
  tft.fillScreen(TFT_BLACK);
  tft.setSwapBytes(true);
}

void Display::setLcdDayBrightness(uint8_t brightness, bool instant) {
  mLcdDayBrightness = brightness;
  startLcdFade(instant);
}
#ifdef OMOTE_HARDWARE_REV5
void Display::setKbdDayBrightness(uint8_t brightness, bool instant) {
  mKbdDayBrightness = brightness;
  startKbdFade(instant);
}
#ifdef OMOTE_KEYBRD_3661
void Display::setLcdNightBrightness(uint8_t brightness, bool instant) {
  mLcdNightBrightness = brightness;
  startLcdFade(instant);
}
void Display::setKbdNightBrightness(uint8_t brightness, bool instant) {
  mKbdNightBrightness = brightness;
  startKbdFade(instant);
}
#else
void Display::setLcdNightBrightness(uint8_t brightness, bool instant) {}
void Display::setKbdNightBrightness(uint8_t brightness, bool instant) {}
#endif
#else
void Display::setKbdDayBrightness(uint8_t brightness, bool instant) {}
void Display::setLcdNightBrightness(uint8_t brightness, bool instant) {}
void Display::setKbdNightBrightness(uint8_t brightness, bool instant) {}
#endif

uint8_t Display::getLcdDayBrightness() { return mLcdDayBrightness; }
uint8_t Display::getKbdDayBrightness() { return mKbdDayBrightness; }
uint8_t Display::getLcdNightBrightness() { return mLcdNightBrightness; }
uint8_t Display::getKbdNightBrightness() { return mKbdNightBrightness; }

void Display::setCurrentLcdBrightness(uint8_t brightness) {
  mLcdBrightness = brightness;
  auto duty = static_cast<int>(mLcdBrightness);
  if (duty < 255)
    ledcWrite((gpio_num_t)mBacklightPin, duty);
  else
    ledc_stop(LEDC_SPEED_MODE, LCD_BACKLIGHT_LEDC_CHANNEL, 255);
}

#ifdef OMOTE_KEYBRD_3661
void Display::setCurrentKbdBrightness(uint8_t brightness) {
  mKbdBrightness = brightness;
  auto duty = static_cast<int>(mKbdBrightness);
  if (duty < 255)
    ledcWrite(KBD_BL, duty);
  else
    ledc_stop(LEDC_LOW_SPEED_MODE, KBD_BACKLIGHT_LEDC_CHANNEL, 255);
}
#else
void Display::setCurrentKbdBrightness(uint8_t brightness) {}
#endif

void Display::turnOff() {
#if defined(OMOTE_HARDWARE_REV5)
  digitalWrite(KBD_BL, LOW);
#endif
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
    // mHaveTouch = false;
    data->state = LV_INDEV_STATE_PRESSED;
    data->point.x = mTouchX;
    data->point.y = mTouchY;
    mTouchPoint = {mTouchX, mTouchY};
    mTouchEvent->notify(mTouchPoint);
  } else {
    data->state = LV_INDEV_STATE_RELEASED;
  }
}

void Display::startLcdFade(bool instant, uint16_t delay) {
  if(xSemaphoreTake(mFadeLcdTaskMutex, 0)) {
    // Only Create Task if it is needed
    if (mDisplayLcdFadeTask == nullptr) {
      uint8_t targetBrightness;
      if (mIsAsleep)
        targetBrightness = 0;
      else {
        if (mIsDay)
          targetBrightness = mLcdDayBrightness;
        else
          targetBrightness = mLcdNightBrightness;
      }

      if (mLcdBrightness != targetBrightness) {
        // calculate delta needed to give consistent 300ms (30 step) fade
        float startBrightness = mLcdBrightness;
        float delta = (targetBrightness - mLcdBrightness);
        if (!instant)
          delta /= 30.0f;
        mLcdArgs = {delta, startBrightness, targetBrightness, delay};
        //Serial.printf("Start LCD fade, start:%f, target:%i, delta:%f, delay:%i\r\n", startBrightness, targetBrightness, delta, delay);
        xTaskCreate(&Display::fadeLcdImpl, "Display Fade Task", 1024, &mLcdArgs, 5, // stack needs to be 2048 for printf use
                      &mDisplayLcdFadeTask);
        }
      }
    xSemaphoreGive(mFadeLcdTaskMutex);
  }
}

void Display::fadeLcdImpl(void *passedArgs) {
  thread_args args = *(thread_args *)passedArgs;
  float brightness = args.startBrightness;
  uint8_t targetBrightness = args.targetBrightness;
  float delta = args.delta;
  uint16_t delay = args.delay;
  //Serial.printf("Impl LCD fade, start:%f, target:%i, delta:%f, delay:%i\r\n", brightness, targetBrightness, delta, delay);
  vTaskDelay(delay / portTICK_PERIOD_MS); 

  do {
    brightness += delta;
    bool overRange = (delta > 0) ? (brightness > targetBrightness) : (brightness < targetBrightness);
    if (overRange)
      getInstance()->setCurrentLcdBrightness(targetBrightness);
    else
      getInstance()->setCurrentLcdBrightness((uint8_t)brightness);
    //Serial.printf("%f\r\n", brightness);
    vTaskDelay(10 / portTICK_PERIOD_MS); // 10 miliseconds between steps
  } while (getInstance()->mLcdBrightness != targetBrightness);
  // Serial.println("Finished LCD fade");

  xSemaphoreTake(getInstance()->mFadeLcdTaskMutex, portMAX_DELAY);
  getInstance()->mDisplayLcdFadeTask = nullptr;
  xSemaphoreGive(getInstance()->mFadeLcdTaskMutex);
  // Serial.println("Deleting fade task");
  vTaskDelete(nullptr); // Delete Fade Task
}

#ifdef OMOTE_HARDWARE_REV5
void Display::startKbdFade(bool instant, uint16_t delay) {
  if(xSemaphoreTake(mFadeKbdTaskMutex, 0)) {
   // Only Create Task if it is needed
    if (mDisplayKbdFadeTask == nullptr) {
      uint8_t targetBrightness;
      if (mIsAsleep)
        targetBrightness = 0;
      else {
        if (mIsDay)
          targetBrightness = mKbdDayBrightness;
        else
          targetBrightness = mKbdNightBrightness;
      }
      
      if (mKbdBrightness != targetBrightness) {
        // calculate delta needed to give consistent 300ms (30 step) fade
        float startBrightness = mKbdBrightness;
        float delta = (targetBrightness - mKbdBrightness);
        if (!instant)
          delta /= 30.0f;
        mKbdArgs = {delta, startBrightness, targetBrightness, delay};
        //Serial.printf("Start KBD fade, start:%f, target:%i, delta:%f, delay:%i\r\n", startBrightness, targetBrightness, delta, delay);
        xTaskCreate(&Display::fadeKbdImpl, "Keyboard Fade Task", 1024, &mKbdArgs, 5, // stack needs to be 2048 for printf use
                      &mDisplayKbdFadeTask);
        }
    }
    xSemaphoreGive(mFadeKbdTaskMutex);
  }
}

void Display::fadeKbdImpl(void *passedArgs) {
  thread_args args = *(thread_args *)passedArgs;
  float brightness = args.startBrightness;
  uint8_t targetBrightness = args.targetBrightness;
  float delta = args.delta;
  uint16_t delay = args.delay;
  //Serial.printf("Impl KBD fade, start:%f, target:%i, delta:%f, delay:%i\r\n", brightness, targetBrightness, delta, delay);
  vTaskDelay(delay / portTICK_PERIOD_MS);

  do {
    brightness += delta;
    bool overRange = (delta > 0) ? (brightness > targetBrightness) : (brightness < targetBrightness);
    if (overRange)
      getInstance()->setCurrentKbdBrightness(targetBrightness);
    else
      getInstance()->setCurrentKbdBrightness((uint8_t)brightness);
    //Serial.printf("%f\r\n", brightness);
    vTaskDelay(10 / portTICK_PERIOD_MS); // 10 miliseconds between steps
  } while (getInstance()->mKbdBrightness != targetBrightness);
  //Serial.println("Finished KBD fade");

  xSemaphoreTake(getInstance()->mFadeKbdTaskMutex, portMAX_DELAY);
  getInstance()->mDisplayKbdFadeTask = nullptr;
  xSemaphoreGive(getInstance()->mFadeKbdTaskMutex);
  // Serial.println("Deleting fade task");
  vTaskDelete(nullptr); // Delete Fade Task
}
#else
void Display::fadeKbdImpl(void *) {}
void Display::startKbdFade(bool instant, uint16_t delay) {}
#endif

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
  // tft.writePixelsDMA((uint16_t *)pixelMap, w * h, true);
  tft.pushPixelsDMA((uint16_t *)pixelMap, w * h);
  tft.endWrite();

  lv_display_flush_ready(disp);
}
