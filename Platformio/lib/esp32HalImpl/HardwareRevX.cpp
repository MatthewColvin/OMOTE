#include "HardwareRevX.hpp"

#include "Esp32Logger.hpp"
#include "Hardware/KeyPressAbstract.hpp"
#include "IRTransceiver.hpp"
#include "display.hpp"
#include "esp32WebSocket.hpp"
#include "observerHandles.hpp"
#include "wifihandler.hpp"

void HardwareRevX::initIO() {
  // Button Pin Definition

  // Power Pin Definition
  pinMode(CRG_STAT, INPUT_PULLUP);

  // IR Pin Definition
  pinMode(IR_RX, INPUT);
  pinMode(IR_LED, OUTPUT);
  pinMode(IR_VCC, OUTPUT);
#if defined(OMOTE_KEYBRD_3661)
  digitalWrite(IR_LED, LOW); // HIGH on - LOW off
#else
  digitalWrite(IR_LED, HIGH); // HIGH off - LOW on
#endif
  IR_VCC_OFF;

  // LCD Pin Definition
  pinMode(LCD_EN, OUTPUT);
  LCD_EN_OFF;
  pinMode(LCD_BL, OUTPUT);
  LCD_BL_OFF;

  // Other Pin Definition
  pinMode(ACC_INT, INPUT);
  pinMode(USER_LED, OUTPUT);
  digitalWrite(USER_LED, LOW);

  // Release GPIO hold in case we are coming out of standby
  gpio_hold_dis((gpio_num_t)LCD_EN);
  gpio_hold_dis((gpio_num_t)LCD_BL);
  gpio_deep_sleep_hold_dis();
}

HardwareRevX::HardwareRevX() : HardwareAbstract() {}

HardwareRevX::WakeReason getWakeReason() {
  // Find out wakeup cause
  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT1) {
    if (esp_sleep_get_ext1_wakeup_status() == (1ULL << ACC_INT))
      return HardwareRevX::WakeReason::IMU;
    else
      return HardwareRevX::WakeReason::KEYPAD;
  } else {
    return HardwareRevX::WakeReason::RESET;
  }
}

void HardwareRevX::init() {
  // Make sure ESP32 is running at full speed
  setCpuFrequencyMhz(240);
  mWakeupReason = getWakeReason();
  initIO();

  Serial.begin(115200);

  mDisplay = Display::getInstance();

  mWifiHandler = wifiHandler::getInstance();

  mWifiHandler->mqttRestoreCredentials();
  mWifiHandler->setupMqttBroker();

  // TODO Could IR be a weak ref only used when needed then deallocate?
  mIr = std::make_shared<IRTransceiver>(logger());

  mBattery = std::make_shared<Battery>(ADC_BAT, CRG_STAT);

  restorePreferences();
  mStandbyTimer = getSleepTimeout();

  mTouchHandler.SetNotification(mDisplay->TouchNotification());
  mTouchHandler = [this](auto aTouchPoint) {
    // When we get touches reset sleep timeout
    mStandbyTimer = this->getSleepTimeout();
  };

  setupIMU();

  UI::observerHandles::registerTextHandle(BATT_STATUS, OBSERVER_BUF_SIZE, "");
  UI::observerHandles::registerTextHandle(WIFI_STATUS, OBSERVER_BUF_SIZE, "");
  UI::observerHandles::registerIntHandle(SOC_STATUS, 0);

  debugPrint("Finished RevX Hardware Setup in %dms", millis());
}

void HardwareRevX::debugPrint(const char *fmt, ...) {
  char result[100];
  va_list arguments;

  va_start(arguments, fmt);
  vsnprintf(result, 100, fmt, arguments);
  va_end(arguments);

  Serial.print(result);
}

std::unique_ptr<LoggingInterface> HardwareRevX::logger() {
  return std::make_unique<ESP32Logger>();
}

std::shared_ptr<wifiHandlerInterface> HardwareRevX::wifi() {
  return mWifiHandler;
}

std::shared_ptr<BatteryInterface> HardwareRevX::battery() { return mBattery; }

std::shared_ptr<DisplayAbstract> HardwareRevX::display() { return mDisplay; }

std::shared_ptr<KeyPressAbstract> HardwareRevX::keys() { return mKeys; }

std::shared_ptr<IRInterface> HardwareRevX::ir() { return mIr; }

std::shared_ptr<SystemStatsInterface> HardwareRevX::stats() {
  if (!mStats) {
    mStats = std::make_shared<EspStats>();
  }
  return mStats;
}

std::shared_ptr<webSocketInterface> HardwareRevX::webSocket() {
  return std::make_shared<esp32WebSocket>(mWifiHandler, std::make_unique<ESP32Logger>());
  return nullptr;
}

std::shared_ptr<LittleFsInterface> HardwareRevX::littleFs() {
  return mLittleFs;
};

std::chrono::milliseconds HardwareRevX::execTime() {
  return std::chrono::milliseconds(millis());
}

bool HardwareRevX::activityDetection() {
  bool activityDetected = false;
  static int accXold = mIMU.readFloatAccelX() * 1000;
  static int accYold = mIMU.readFloatAccelY() * 1000;
  static int accZold = mIMU.readFloatAccelZ() * 1000;
  static int accXbuf[4], accYbuf[4], accZbuf[4];
  static int motion = 0;
  static uint8_t bufferIndex = 0;

  accXbuf[bufferIndex] = mIMU.readFloatAccelX() * 1000;
  accYbuf[bufferIndex] = mIMU.readFloatAccelY() * 1000;
  accZbuf[bufferIndex] = mIMU.readFloatAccelZ() * 1000;

  bufferIndex++;
  if (bufferIndex >= 4) {
    bufferIndex = 0;
    int accX = (accXbuf[0] + accXbuf[1] + accXbuf[2] + accXbuf[3]) / 4;
    int accY = (accYbuf[0] + accYbuf[1] + accYbuf[2] + accYbuf[3]) / 4;
    int accZ = (accZbuf[0] + accZbuf[1] + accZbuf[2] + accZbuf[3]) / 4;
    // determine motion value as da/dt
    motion = (abs(accXold - accX) + abs(accYold - accY) + abs(accZold - accZ));
    // Store the current acceleration and time
    accXold = accX;
    accYold = accY;
    accZold = accZ;
    if (motion > MOTION_THRESHOLD)
      activityDetected = true;
  }
  return activityDetected;
}

char HardwareRevX::getCurrentDevice() { return mCurrentDevice; }

void HardwareRevX::setCurrentDevice(char currentDevice) {
  this->mCurrentDevice = currentDevice;
}

bool HardwareRevX::getWakeupByIMUEnabled() { return mWakeupByIMUEnabled; }

void HardwareRevX::setWakeupByIMUEnabled(bool wakeupByIMUEnabled) {
  this->mWakeupByIMUEnabled = wakeupByIMUEnabled;
}

uint32_t HardwareRevX::getSleepTimeout() { return mSleepTimeout; }

void HardwareRevX::setSleepTimeout(uint32_t sleepTimeout) {
  this->mSleepTimeout = sleepTimeout;
  mStandbyTimer = sleepTimeout;
}

void HardwareRevX::saveSettings() {
  // Save settings to internal flash memory
  mPreferences.begin("settings", false);
  mPreferences.putBool("wkpByIMU", mWakeupByIMUEnabled);
  mPreferences.putUChar("lcdDayBright", mDisplay->getLcdDayBrightness());
  mPreferences.putUChar("lcdNightBright", mDisplay->getLcdNightBrightness());
  mPreferences.putUChar("kbdDayBright", mDisplay->getKbdDayBrightness());
  mPreferences.putUChar("kbdNightBright", mDisplay->getKbdNightBrightness());
  mPreferences.putUChar("currentDevice", mCurrentDevice);
  mPreferences.putUInt("sleepTimeout", mSleepTimeout);
  if (!mPreferences.getBool("alreadySetUp"))
    mPreferences.putBool("alreadySetUp", true);
  mPreferences.end();
  // Serial.println("Settings Saved");
}

void HardwareRevX::enterSleep() {
  // Configure IMU
  uint8_t intDataRead;
  mIMU.readRegister(&intDataRead, LIS3DH_INT1_SRC); // clear interrupt
  configIMUInterrupts();
  mIMU.readRegister(&intDataRead,
                    LIS3DH_INT1_SRC); // really clear interrupt

  // Prepare IO states
  digitalWrite(LCD_DC, LOW); // LCD control signals off
  digitalWrite(LCD_CS, LOW);

  sleepDisplayPins();

  digitalWrite(LCD_EN, HIGH); // LCD logic off
  digitalWrite(LCD_BL, HIGH); // LCD backlight off
  pinMode(CRG_STAT, INPUT);   // Disable Pull-Up
  digitalWrite(IR_VCC, LOW);  // IR Receiver off

  configPinsForSleepInterrupts();

  // Force display pins to high impedance
  // Without this the display might not wake up from sleep
  pinMode(LCD_BL, INPUT);
  pinMode(LCD_EN, INPUT);
  gpio_hold_en((gpio_num_t)LCD_BL);
  gpio_hold_en((gpio_num_t)LCD_EN);
  gpio_deep_sleep_hold_en();

  enableWakeupByPin();

  delay(100);
  // Sleep
  esp_deep_sleep_start();
}

void HardwareRevX::enableWakeupByPin() {
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
}

void HardwareRevX::configIMUInterrupts() {
  uint8_t dataToWrite = 0;

  // LIS3DH_INT1_CFG
  // dataToWrite |= 0x80;//AOI, 0 = OR 1 = AND
  // dataToWrite |= 0x40;//6D, 0 = interrupt source, 1 = 6 direction source
  // Set these to enable individual axes of generation source (or direction)
  //  -- high and low are used generically
  dataToWrite |= 0x20; // Z high
  // dataToWrite |= 0x10;//Z low
  dataToWrite |= 0x08; // Y high
  // dataToWrite |= 0x04;//Y low
  dataToWrite |= 0x02; // X high
  // dataToWrite |= 0x01;//X low
  if (mWakeupByIMUEnabled)
    mIMU.writeRegister(LIS3DH_INT1_CFG, 0b00101010);
  else
    mIMU.writeRegister(LIS3DH_INT1_CFG, 0b00000000);

  // LIS3DH_INT1_THS
  dataToWrite = 0;
  // Provide 7 bit value, 0x7F always equals max range by accelRange setting
  dataToWrite |= 0x45;
  mIMU.writeRegister(LIS3DH_INT1_THS, dataToWrite);

  // LIS3DH_INT1_DURATION
  dataToWrite = 0;
  // minimum duration of the interrupt
  // LSB equals 1/(sample rate)
  dataToWrite |= 0x00; // 1 * 1/50 s = 20ms
  mIMU.writeRegister(LIS3DH_INT1_DURATION, dataToWrite);

  // LIS3DH_CTRL_REG5
  // Int1 latch interrupt and 4D on  int1 (preserve fifo en)
  mIMU.readRegister(&dataToWrite, LIS3DH_CTRL_REG5);
  dataToWrite &= 0xF3; // Clear bits of interest
  dataToWrite |= 0x08; // Latch interrupt (Cleared by reading int1_src)
  // dataToWrite |= 0x04; //Pipe 4D detection from 6D recognition to int1?
  mIMU.writeRegister(LIS3DH_CTRL_REG5, dataToWrite);

  // LIS3DH_CTRL_REG6
  configIMUInterruptPolarity();

  // LIS3DH_CTRL_REG3
  // Choose source for pin 1
  dataToWrite = 0;
  // dataToWrite |= 0x80; //Click detect on pin 1
  dataToWrite |= 0x40; // AOI1 event (Generator 1 interrupt on pin 1)
  dataToWrite |= 0x20; // AOI2 event ()
  // dataToWrite |= 0x10; //Data ready
  // dataToWrite |= 0x04; //FIFO watermark
  // dataToWrite |= 0x02; //FIFO overrun
  mIMU.writeRegister(LIS3DH_CTRL_REG3, dataToWrite);
}

void HardwareRevX::configIMUInterruptPolarity() {
  mIMU.writeRegister(LIS3DH_CTRL_REG6, 0x00); // For active-high interrupt
}

void HardwareRevX::restorePreferences() {
  // Restore settings from internal flash memory
  int lcd_day_backlight_brightness = 255;
  int lcd_night_backlight_brightness = 255;
  int kbd_day_backlight_brightness = 255;
  int kbd_night_backlight_brightness = 255;
  mPreferences.begin("settings", false);
  if (mPreferences.getBool("alreadySetUp")) {
    mWakeupByIMUEnabled = mPreferences.getBool("wkpByIMU");
    lcd_day_backlight_brightness = mPreferences.getUChar("lcdDayBright");
    lcd_night_backlight_brightness = mPreferences.getUChar("lcdNightBright");
    kbd_day_backlight_brightness = mPreferences.getUChar("kbdDayBright");
    kbd_night_backlight_brightness = mPreferences.getUChar("kbdNightBright");
    mCurrentDevice = mPreferences.getUChar("currentDevice");
    mSleepTimeout = mPreferences.getUInt("sleepTimeout");
    // setting the default to prevent a 0ms sleep timeout
    if (mSleepTimeout == 0) {
      mSleepTimeout = SLEEP_TIMEOUT;
    }
  }
  mPreferences.end();

  if (lcd_day_backlight_brightness < 10)
    lcd_day_backlight_brightness = 10;
  if (lcd_night_backlight_brightness < 10)
    lcd_night_backlight_brightness = 10;
  mDisplay->setLcdDayBrightness(lcd_day_backlight_brightness);
  mDisplay->setLcdNightBrightness(lcd_night_backlight_brightness);
  mDisplay->setKbdDayBrightness(kbd_day_backlight_brightness);
  mDisplay->setKbdNightBrightness(kbd_night_backlight_brightness);
}

void HardwareRevX::setupIMU() {
  // Setup hal
  // Hz.  Can be: 0,1,10,25,50,100,200,400,1600,5000 Hz
  // Note: 10ms per sample allows guaranteed clean sample between I2C reads
  mIMU.settings.accelSampleRate = 100;
  // Max G force readable.  Can be: 2, 4, 8, 16
  mIMU.settings.accelRange = 2;
  mIMU.settings.adcEnabled = 0;
  mIMU.settings.tempEnabled = 0;
  mIMU.settings.xAccelEnabled = 1;
  mIMU.settings.yAccelEnabled = 1;
  mIMU.settings.zAccelEnabled = 1;
  mIMU.begin();
  uint8_t intDataRead;
  mIMU.readRegister(&intDataRead, LIS3DH_INT1_SRC); // clear interrupt
}

void HardwareRevX::startTasks() {}

void HardwareRevX::loopHandler() {
  mWifiHandler->mqttSync();

  mIr->loopHandleRx();

  mStandbyTimer < 2000 ? mDisplay->sleep() : mDisplay->wake();

  // Blink debug LED at 1 Hz
  digitalWrite(USER_LED, millis() % 1000 > 500);

  // Refresh IMU data at 10Hz
  static unsigned long IMUTaskTimer = millis();
  if (millis() - IMUTaskTimer >= 25) {
    // Calculate time to standby
    mStandbyTimer -= 25;
    if (mStandbyTimer < 0)
      mStandbyTimer = 0;

    if (activityDetection())
      mStandbyTimer = mSleepTimeout;

    if (keyboardScan())
      mStandbyTimer = mSleepTimeout;

    uint16_t visPlusIrLevel, irLevel;
    if (lightSensorScan(visPlusIrLevel, irLevel)) {
      // Serial.printf("ll:%d\r\n", irLevel);
      // use IR as still responds to ambient light level but
      //  less sensitive to keypad illumination
      updateBacklightMode(irLevel);
    }

    mDisplay->getTouchData(); // trigger read here to keep all I2C accesses
                              // together

    static uint16_t secCount = 20; // update immediately on power up
    if (secCount++ >= 20) {
      Serial.printf("Heap: %.2f%% free of %dkB, Pram: %.2f%% free of %dkB\r\n",
                    (100.0f * ESP.getFreeHeap()) / ESP.getHeapSize(), ESP.getHeapSize() / 1024,
                    (100.0f * ESP.getFreePsram()) / ESP.getPsramSize(), ESP.getPsramSize() / 1024);

      secCount = 0;
      int32_t iSoc = mBattery->getPercentage();
      if (iSoc > 99)
        iSoc = 99;
      UI::observerHandles::setInt(SOC_STATUS, iSoc);

      if (mBattery->isConnected())
        UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_USB);
      else {
        if (iSoc < 13)
          UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_EMPTY);
        else if (iSoc < 38)
          UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_1);
        else if (iSoc < 63)
          UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_2);
        else if (iSoc < 88)
          UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_3);
        else
          UI::observerHandles::setText(BATT_STATUS, LV_SYMBOL_BATTERY_FULL);
      }

      wifiHandlerInterface::wifiStatus wifiStatus = mWifiHandler->GetStatus();
      if (wifiStatus.isConnected)
        UI::observerHandles::setText(WIFI_STATUS, LV_SYMBOL_WIFI);
      else
        UI::observerHandles::setText(WIFI_STATUS, "");
      // Serial.printf("IR:%d, V+IR:%d, SOC:%.0f, Volts:%.2f\r\n",irLevel,
      // visPlusIrLevel, soc, voltage);
    }

    if (mStandbyTimer == 0) {
      Serial.println("Entering Sleep Mode. Goodbye.");
      enterSleep();
    }
    IMUTaskTimer = millis();
  }
}
