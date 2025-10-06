#include "HardwareRevX.hpp"
#include "Esp32Logger.hpp"
#include "Hardware/KeyPressAbstract.hpp"
#include "IRTransceiver.hpp"
#include "display.hpp"
#include "driver/rtc_io.h"
#include "esp32WebSocket.hpp"
#include "observerHandles.hpp"
#include "wifihandler.hpp"
#include <Wire.h>

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
  gpio_hold_dis((gpio_num_t)ACC_INT);
#if defined OMOTE_HARDWARE_REV5
  gpio_hold_dis((gpio_num_t)TCA_INT);
  gpio_hold_dis((gpio_num_t)SD_EN);
#endif
#if defined(OMOTE_KEYBRD_3661)
  gpio_hold_dis((gpio_num_t)3);
#else
  gpio_hold_dis((gpio_num_t)LCD_BL);
#endif
  gpio_deep_sleep_hold_dis();
}

HardwareRevX::HardwareRevX() : HardwareAbstract(), mLogger(std::make_unique<LoggingInterface>()) {
  mLogger->setLogModule(LogModule::General);
  mIMU_new = std::make_shared<LIS3DH_IMU>(mIMU);
}

HardwareRevX::WakeReason getWakeReason() {
  // Find out wakeup cause
  // Serial.printf("reset reason: %i, wake reason: %i\r\n", esp_reset_reason(), esp_sleep_get_wakeup_cause());

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_TIMER)
    return HardwareRevX::WakeReason::TIMER;

  if (esp_sleep_get_wakeup_cause() == ESP_SLEEP_WAKEUP_EXT0)
    return HardwareRevX::WakeReason::CHARGER;

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

  // Make sure time is zeroed if booting from deep sleep due to poor accuracy
  // of internal oscillator.
  {
    timeval tv(0, 0);
    timezone tz(0, 0);
    settimeofday(&tv, &tz);
  }

  mLogger->setLogModule(LogModule::General);
  if (mLogger->isPrintWanted(LogLevel::Info)) {
    mLogStream << "Wake up due to " << magic_enum::enum_name(mWakeupReason);
    mLogger->log(LogLevel::Info, mLogStream);
  }

  mDisplay = Display::getInstance();

  mWifiHandler = wifiHandler::getInstance();

  mWifiHandler->mqttRestoreCredentials();
  mWifiHandler->setupMqttBroker();

  mWifiHandler->ntpRestoreCredentials();
  mWifiHandler->setupNtp();

  mWifiHandler->ftpRestoreCredentials();

  // TODO Could IR be a weak ref only used when needed then deallocate?
  mIr = std::make_shared<IRTransceiver>(logger());

  // mBattery = std::make_shared<Battery>(ADC_BAT, CRG_STAT);
  //  mBattery->writeCustomModel();

  restorePreferences();
  mStandbyTimer = getSleepTimeout();

  mTouchHandler.SetNotification(mDisplay->TouchNotification());
  mTouchHandler = [this](auto aTouchPoint) {
    // When we get touches reset sleep timeout
    mStandbyTimer = this->getSleepTimeout();
  };

  mIMU_new->setup();

  UI::observerHandles::registerTextHandle(GENERAL_STATUS, OBSERVER_BUF_SIZE, "");

  mLogger->setLogModule(LogModule::General);
  if (mLogger->isPrintWanted(LogLevel::Info)) {
    mLogStream << "Finished RevX Hardware Setup in " << millis() << "ms";
    mLogger->log(LogLevel::Info, mLogStream);
  }

  // mDisplay->startFade(50); // allow time for LCD init to complete before bringing up backlight
  // fade triggered on first light sensor data
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
}

std::shared_ptr<LIS3DH_IMU> HardwareRevX::imu() {
  return mIMU_new;
}

std::chrono::milliseconds HardwareRevX::execTime() {
  return std::chrono::milliseconds(millis());
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
}

bool HardwareRevX::getLightSleepEnabled() { return mLightSleepEnabled; }

void HardwareRevX::setLightSleepEnabled(bool lightSleepEnabled) {
  this->mLightSleepEnabled = lightSleepEnabled;
}

uint32_t HardwareRevX::getLightSleepTimeout() { return mLightSleepTimeout; }

void HardwareRevX::setLightSleepTimeout(uint32_t lightSleepTimeout) {
  this->mLightSleepTimeout = lightSleepTimeout;
}

void HardwareRevX::setInScene(bool inScene) {
  this->mInScene = inScene;
}

void HardwareRevX::saveSettings() {
  // Save settings to internal flash memory
  mPreferences.begin("settings", false);
  mPreferences.putBool("wkpByIMU", mWakeupByIMUEnabled);
  mPreferences.putBool("lightSlpEn", mLightSleepEnabled);
  mPreferences.putUChar("lcdDayBright", mDisplay->getLcdDayBrightness());
  mPreferences.putUChar("lcdNightBright", mDisplay->getLcdNightBrightness());
  mPreferences.putUChar("kbdDayBright", mDisplay->getKbdDayBrightness());
  mPreferences.putUChar("kbdNightBright", mDisplay->getKbdNightBrightness());
  mPreferences.putUChar("currentDevice", mCurrentDevice);
  mPreferences.putUInt("sleepTimeout", mSleepTimeout);
  mPreferences.putUInt("LgtSlpTimeout", mLightSleepTimeout);
  if (!mPreferences.getBool("alreadySetUp"))
    mPreferences.putBool("alreadySetUp", true);
  mPreferences.end();

  mLogger->setLogModule(LogModule::Display);
  if (mLogger->isPrintWanted(LogLevel::Info))
    mLogger->log(LogLevel::Info, "Settings Saved");
}

void HardwareRevX::enterSleep(SleepMode mode, uint32_t duration) {
  /*
  Light sleep implementation is a bit crude, rather than making use of all the features
  of light sleep to allow automatic pin changes, allow minimal disruption to initialised
  peripherals and things like WiFi/auto sleep it instead goes for maximum simularity to
  the deep sleep code.

  Did try using the full light sleep functions but supply currents were worse due to pin
  config errors.  It's going to take more work to sort than I currently have time for.

  Light sleep current on the S3 still seems higher than it should be (1mA rather than 350uA),
  not sure why but suspicious that the CPU isn't powering down correctly (disabling CPU power
  down makes no difference!)  This would give 650uA increase on S3 and would explain the high current.
  It's also possible that it's due to a pin config error but if so can't find it.
  Testing with a bare STM32-S3FH4R2 with the ESP demo code gave around 700uA with the same config -
  a bit better but still not as low as it should be.
  Also worth noting that disabling USB for debug and terminal output in menuconfig actually makes
  things worse (even though it isn't being used)!  Seems to mess up the state of the USB lines
  during lightsleep causing backpowering the LCD and increasing current to 2.25mA.

  Current implementation gives:
    Deep sleep boot to slect scene menu: ~500ms
    Deep sleep boot to 1page scene:      ~700ms
    Deep sleep boot to 4page scene:      ~1250ms
    Light sleep boot to any scene:       ~250ms

    Active current (3661):      ~140mA
    light sleep current (3661): ~1mA
    Deep sleep current (3661):  ~50uA
  */

  // Configure IMU
  mIMU.settings.accelSampleRate = 50; // 100Hz seems to give 300uA current spikes every 4th conversion??
  mIMU.applySettings();
  uint8_t intDataRead;
  mIMU.readRegister(&intDataRead, LIS3DH_INT1_SRC); // clear interrupt
  mIMU_new->configIMUInterrupts(mWakeupByIMUEnabled);
  mIMU.readRegister(&intDataRead,
                    LIS3DH_INT1_SRC); // really clear interrupt
  // Power down modem
  WiFi.disconnect();
  WiFi.mode(WIFI_OFF);

  // Prepare IO states
  pinMode(LCD_DC, INPUT_PULLDOWN); // LCD control signals off
  pinMode(LCD_CS, INPUT_PULLDOWN);

  sleepDisplayPins();

  pinMode(LCD_EN, INPUT_PULLUP);
#if defined(OMOTE_KEYBRD_3661)
  pinMode(LCD_BL, INPUT_PULLDOWN);
  pinMode(3, INPUT_PULLUP);
  gpio_hold_en((gpio_num_t)3);
#else
  pinMode(LCD_BL, INPUT_PULLUP);
  gpio_hold_en((gpio_num_t)LCD_BL);
#endif
#if defined OMOTE_HARDWARE_REV5
  pinMode(KBD_BL, INPUT_PULLDOWN);
#endif
  pinMode(CRG_STAT, INPUT_PULLDOWN); // Disable Pull-Up

  // Following pins don't get reconfigured by default on light sleep wake so use light sleep redef.
  // Possibly should use this for all pins to simplify re-init but couldn't get to work right
  // pinMode(IR_VCC, INPUT_PULLDOWN);
  gpio_sleep_set_direction((gpio_num_t)IR_VCC, GPIO_MODE_INPUT);
  gpio_sleep_set_pull_mode((gpio_num_t)IR_VCC, GPIO_PULLDOWN_ONLY);
  gpio_sleep_sel_en((gpio_num_t)IR_VCC);
  // pinMode(USER_LED, INPUT_PULLDOWN);
  gpio_sleep_set_direction((gpio_num_t)USER_LED, GPIO_MODE_INPUT);
  gpio_sleep_set_pull_mode((gpio_num_t)USER_LED, GPIO_PULLDOWN_ONLY);
  gpio_sleep_sel_en((gpio_num_t)USER_LED);

  configPinsForSleepInterrupts();

  // Isolate any pins that need to remain high in deep sleep from the GPIO power domain
  // Without this they will backfeed and each will add 100uA to the deep sleep current
  gpio_hold_en((gpio_num_t)LCD_EN);
  gpio_hold_en((gpio_num_t)ACC_INT);
#if defined OMOTE_HARDWARE_REV5
  gpio_hold_en((gpio_num_t)TCA_INT);
  gpio_hold_en((gpio_num_t)SD_EN);
#endif
  gpio_deep_sleep_hold_en();

  Wire.end();
  pinMode(SCL, OUTPUT);
  digitalWrite(SCL, LOW);
  delay(2);
  pinMode(SDA, OUTPUT);
  digitalWrite(SDA, LOW);
  pinMode(SCL, INPUT_PULLDOWN);
  pinMode(SDA, INPUT_PULLDOWN);

  // Serial.end();
  gpio_sleep_set_direction((gpio_num_t)TX, GPIO_MODE_INPUT);
  gpio_sleep_set_pull_mode((gpio_num_t)TX, GPIO_PULLDOWN_ONLY);
  gpio_sleep_sel_en((gpio_num_t)TX);
  gpio_sleep_set_direction((gpio_num_t)RX, GPIO_MODE_INPUT);
  gpio_sleep_set_pull_mode((gpio_num_t)RX, GPIO_PULLDOWN_ONLY);
  gpio_sleep_sel_en((gpio_num_t)RX);

  esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
  enableWakeupByPin();

  if ((mode == SleepMode::LIGHT_SLEEP_WAKE_ON_CHG) || (mode == SleepMode::LIGHT_SLEEP_WAKE_ON_NOCHG) || (mode == SleepMode::LIGHT_DEEP_SLEEP)) {
    if ((mode == SleepMode::LIGHT_SLEEP_WAKE_ON_NOCHG) || (mode == SleepMode::LIGHT_SLEEP_WAKE_ON_CHG)) { // battery calibration
      rtc_gpio_pullup_en((gpio_num_t)CRG_STAT);
      esp_sleep_enable_ext0_wakeup((gpio_num_t)CRG_STAT, mode == SleepMode::LIGHT_SLEEP_WAKE_ON_NOCHG ? HIGH : LOW);
    }
    if (duration == 0)
      esp_sleep_enable_timer_wakeup(((uint64_t)mLightSleepTimeout) * 1000); // light sleep duration
    else
      esp_sleep_enable_timer_wakeup(((uint64_t)duration) * 1000); // light sleep duration
    delay(10);
    esp_light_sleep_start();
  } else {
    delay(10);
    esp_deep_sleep_start();
  }

  // if deep sleep will restart at main, will only continue here if light sleep
  rtc_gpio_deinit((gpio_num_t)CRG_STAT);
  lightSleepWakeReint(mode);
}

void HardwareRevX::lightSleepWakeReint(SleepMode mode) {
  mWakeTime = millis();
  mWakeupReason = getWakeReason();

  mLogger->setLogModule(LogModule::General);

  if ((mWakeupReason == HardwareRevX::WakeReason::TIMER) && (mode == SleepMode::LIGHT_DEEP_SLEEP)) {
    mLogger->info("Timer wakeup, entering deep sleep");
    esp_sleep_disable_wakeup_source(ESP_SLEEP_WAKEUP_ALL);
    enableWakeupByPin();
    delay(10);
    esp_deep_sleep_start();
  }

  gpio_deep_sleep_hold_dis();

  if (mLogger->isPrintWanted(LogLevel::Info)) {
    mLogStream << "Wake from light sleep at " << millis() << " due to " << magic_enum::enum_name(mWakeupReason);
    mLogger->log(LogLevel::Info, mLogStream);
  }

  mWifiHandler->begin();
  mWifiHandler->mqttForceReconnect();

  initIO();

  Wire.begin();

  mIMU.settings.accelSampleRate = 100;
  mIMU.applySettings();

  mDisplay->reInit();

  mIMUTaskTimer = millis();
  mStandbyTimer = getSleepTimeout();
  mIMU_new->setup();

  mLogger->setLogModule(LogModule::General);
  if (mLogger->isPrintWanted(LogLevel::Info)) {
    mLogStream << "Finished RevX Reinit in " << millis() << "ms";
    mLogger->log(LogLevel::Info, mLogStream);
  }
}

void HardwareRevX::enableWakeupByPin() {
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_HIGH);
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
    mLightSleepEnabled = mPreferences.getBool("lightSlpEn");
    lcd_day_backlight_brightness = mPreferences.getUChar("lcdDayBright");
    lcd_night_backlight_brightness = mPreferences.getUChar("lcdNightBright");
    kbd_day_backlight_brightness = mPreferences.getUChar("kbdDayBright");
    kbd_night_backlight_brightness = mPreferences.getUChar("kbdNightBright");
    mCurrentDevice = mPreferences.getUChar("currentDevice");
    mSleepTimeout = mPreferences.getUInt("sleepTimeout");
    mLightSleepTimeout = mPreferences.getUInt("LgtSlpTimeout");
    // setting the default to prevent a 0ms sleep timeout
    if (mSleepTimeout == 0) {
      mSleepTimeout = SLEEP_TIMEOUT;
    }
    if (mLightSleepTimeout == 0) {
      mLightSleepTimeout = LIGHT_SLEEP_TIMEOUT;
    }
  }
  mPreferences.end();

  if (lcd_day_backlight_brightness < 10)
    lcd_day_backlight_brightness = 10;
  if (lcd_night_backlight_brightness < 10)
    lcd_night_backlight_brightness = 10;

  // initialise levels but do not start fade yet
  mDisplay->initBrightnessLevels(lcd_day_backlight_brightness, lcd_night_backlight_brightness,
                                 kbd_day_backlight_brightness, kbd_night_backlight_brightness);
}

void HardwareRevX::startTasks() {}

void HardwareRevX::loopHandler() {
  static int32_t battVoltage = 0;

  mWifiHandler->mqttSync();
  mWifiHandler->ftpSync();

  mWifiHandler->nptSync();

  mIr->loopHandleRx();

  mStandbyTimer < 750 ? mDisplay->sleep() : mDisplay->wake();

  // Blink debug LED at 1 Hz
  digitalWrite(USER_LED, millis() % 1000 > 500);

  // Refresh IMU data at 40Hz
  if (millis() - mIMUTaskTimer >= 25) {
    // Calculate time to standby
    mStandbyTimer -= (millis() - mIMUTaskTimer);
    mIMUTaskTimer = millis();
    if (mStandbyTimer < 0)
      mStandbyTimer = 0;

    if (mIMU_new->activityDetection())
      mStandbyTimer = mSleepTimeout;

    if (keyboardScan())
      mStandbyTimer = mSleepTimeout;

    uint16_t visPlusIrLevel, irLevel;
    if (lightSensorScan(visPlusIrLevel, irLevel)) {
      // use IR as still responds to ambient light level but
      //  less sensitive to keypad illumination
      updateBacklightMode(irLevel);
      mLogger->setLogModule(LogModule::Display);
      if (mLogger->isPrintWanted(LogLevel::Debug)) {
        mLogStream << "Light sensor:" << irLevel;
        mLogger->log(LogLevel::Debug, mLogStream);
      }
    }

    mDisplay->getTouchData(); // trigger read here to keep all I2C accesses
                              // together
    battVoltage = battery()->getVoltage();

    static uint16_t secCount = 20; // update immediately on power up
    if (++secCount >= 20) {        // 500ms
      secCount = 0;

      mLogger->setLogModule(LogModule::Memory);
      if (mLogger->isPrintWanted(LogLevel::Info)) {
        mLogStream.precision(2);
        mLogStream << "Heap:" << (100.0f * ESP.getFreeHeap()) / ESP.getHeapSize() << "% free of "
                   << ESP.getHeapSize() / 1024 << "kB, Pram:" << (100.0f * ESP.getFreePsram()) / ESP.getPsramSize()
                   << "% free of " << ESP.getPsramSize() / 1024 << "kB, Stack min free: "
                   << uxTaskGetStackHighWaterMark(nullptr) << "w";
        mLogger->log(LogLevel::Info, mLogStream);
      }

      if (mStandbyTimer == 0) {
        mLogger->setLogModule(LogModule::General);

        if (mInScene && mLightSleepEnabled) {
          mLogger->info("Entering Light Sleep Mode. Bye");
          enterSleep(SleepMode::LIGHT_DEEP_SLEEP); // light sleep then automatically drop into deep sleep
          secCount = 20;                           // update immediately on power up
        } else {
          mLogger->info("Entering Deep Sleep Mode. Goodbye");
          enterSleep(SleepMode::DEEP_SLEEP);
        }
      }

      // Note - this block is likely to need tweaking, or even disabling, on
      // hardware revs <5 due to the reduced voltage measurement accuracy
      static int lowBattTimer = 0;
      // Serial.printf("Battery Voltage: %imV\r\n", battVoltage);
      // Fairly early stop to limit battery degradation, trips
      // around 5min after hitting 0% SOC
      if (battVoltage < 3450) {
        lowBattTimer++;
        if (lowBattTimer >= 4) { // constantly low for 2s
          mLogger->info("Battery low, entering Deep Sleep");
          enterSleep(SleepMode::DEEP_SLEEP);
        }
      } else {
        lowBattTimer = 0;
      }

      mLogger->setLogModule(LogModule::General);
      if (mLogger->isPrintWanted(LogLevel::Debug)) {
        mLogStream << "Main sensor loop ran at:" << mIMUTaskTimer << "ms, execution time:" << (millis() - mIMUTaskTimer);
        mLogger->log(LogLevel::Debug, mLogStream);
      }
    }
  }
}

void HardwareRevX::updateBacklightMode(uint16_t lightLevel) {
  static bool firstData = true;

  if (firstData) {
    // if first measurement start fade regardless, if already started this will be ignored
    firstData = false;
    mDisplay->startFade(25);
  }
}

bool HardwareRevX::lightSensorScan(uint16_t &visPlusIrLevel, uint16_t &irLevel) {
  static bool firstTime = true;
  if (firstTime) {
    firstTime = false;
    return true;
  } else
    return false;
};
