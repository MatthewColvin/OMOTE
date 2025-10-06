#include "HardwareRev5.hpp"
#include <Adafruit_TCA8418.h>
#include <LittleFS.h>

HardwareRev5::HardwareRev5() : mLogger(std::make_unique<LoggingInterface>()) {
  mLogger->setLogModule(LogModule::General);
}

void listDir(fs::FS &fs, const char *dirname, uint8_t levels) {
  Serial.printf("Listing directory: %s\r\n", dirname);

  File root = fs.open(dirname);
  if (!root) {
    Serial.println("- failed to open directory");
    return;
  }
  if (!root.isDirectory()) {
    Serial.println(" - not a directory");
    return;
  }

  File file = root.openNextFile();
  while (file) {
    if (file.isDirectory()) {
      Serial.printf("  DIR: %s\r\n", file.name());
      if (levels) {
        listDir(fs, file.path(), levels - 1);
      }
    } else {
      Serial.printf("  FILE: %s, SIZE: %i\r\n", file.name(), file.size());
    }
    file = root.openNextFile();
  }
}

void HardwareRev5::init() {
  mLogger->setLogModule(LogModule::LittleFs);

  Serial.begin(115200);

  if (LittleFS.begin(true)) {
    // listDir(LittleFS, "/", 3);
    if (mLogger->isPrintWanted(LogLevel::Info))
      mLogger->log(LogLevel::Info, "Mounted OK");
  } else {
    if (mLogger->isPrintWanted(LogLevel::Error))
      mLogger->log(LogLevel::Error, "Mounted Failed");
  }
  LoggingInterface::restoreSettings();
  HardwareRevX::init();

  mBattery = std::make_shared<BatteryRev5>(ADC_BAT, CRG_STAT);

  static constexpr auto MaxQueueableKeyPresses = 5;

  mKeys = std::make_shared<Keys>();
  setupKeyboard();

  // Bit of a hack as need to disable serial before checking for USB connection due to backfeed
  // but want to initialise serial early (for logging) and also need to check after keypad IC initialised
  // Would be better to restructure initialisation to fix properly (but not right now!)
  Serial.end();
  pinMode(43, OUTPUT);
  digitalWrite(43, LOW);
  pinMode(44, INPUT_PULLDOWN);
  delay(20);
  if (isUsbConnected())
    Serial.begin(115200);

#ifdef OMOTE_KEYBRD_3661
  setupLightSensor();
#endif

  mLogger->setLogModule(LogModule::General);
  if (mLogger->isPrintWanted(LogLevel::Info)) {
    std::stringstream ss;
    ss << "Finished Rev5 Hardware Setup in :" << millis() << "ms";
    mLogger->log(LogLevel::Info, ss);
  }
}

void HardwareRev5::lightSleepWakeReint(SleepMode mode) {
  HardwareRevX::lightSleepWakeReint(mode);
#ifdef OMOTE_KEYBRD_3661
  ltr.reset();
  setupLightSensor();
#endif
}

void HardwareRev5::initIO() {
  HardwareRevX::initIO();
  pinMode(SD_EN, OUTPUT);
  SD_EN_OFF;
  pinMode(KBD_BL, OUTPUT);
  KBD_BL_OFF;
  pinMode(TCA_INT, INPUT);
}

bool HardwareRev5::isUsbConnected() {
// NOTE: due to backfeeding need to disable serial and set the Tx line low for 20ms before calling this
// in order to get an accurate result
#ifdef OMOTE_KEYBRD_3661
  return (keypad.digitalRead(14) == HIGH); // USB_3V3
#else
  return (keypad.digitalRead(13) == HIGH); // USB_3V3
#endif
}

void HardwareRev5::setupKeyboard() {
  if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    mLogger->setLogModule(LogModule::Keys);
    if (mLogger->isPrintWanted(LogLevel::Error))
      mLogger->log(LogLevel::Error, "Keypad TCA8418 not found!");
  }
  keypad.matrix(KEYPAD_ROWS, KEYPAD_COLS);
  keypad.pinMode(5, INPUT_PULLUP); // SW_PWR
  keypad.pinMode(6, INPUT_PULLUP); // SD_DET
#ifdef OMOTE_KEYBRD_3661
  keypad.pinMode(14, INPUT); // USB_3V3
#else
  keypad.pinMode(13, INPUT); // USB_3V3
#endif

  pinMode(TCA_INT, INPUT);
  // don't flush keyboard FIFO if wake due to keypress, will then be processed once config complete
  if (HardwareRevX::getWakeUpReason() != HardwareRevX::WakeReason::KEYPAD) {
    mLogger->setLogModule(LogModule::Keys);
    if (mLogger->isPrintWanted(LogLevel::Debug))
      mLogger->log(LogLevel::Debug, "Flushing FIFO");
    keypad.flush();
  }
  keypad.writeRegister(TCA8418_REG_CFG, 0b00000001);
  keypad.writeRegister(TCA8418_REG_GPI_EM_1, KEYPAD_ROWS_BITMASK);
  keypad.writeRegister(TCA8418_REG_GPI_EM_2, KEYPAD_COLS_BITMASK);
}

#ifdef OMOTE_KEYBRD_3661
void HardwareRev5::setupLightSensor() {
  if (ltr.begin()) {
    ltr.setGain(LTR3XX_GAIN_8);
    ltr.setIntegrationTime(LTR3XX_INTEGTIME_50);
    ltr.setMeasurementRate(LTR3XX_MEASRATE_50);
    mlightSensorInitSuccessful = true;
  } else {
    mLogger->setLogModule(LogModule::Display);
    if (mLogger->isPrintWanted(LogLevel::Error))
      mLogger->log(LogLevel::Error, "Couldn't find LTR-303 sensor!");
  }
}

bool HardwareRev5::lightSensorScan(uint16_t &visPlusIrLevel,
                                   uint16_t &irLevel) {
  static bool firstMeas = true;
  bool retVal = false;
  if (mlightSensorInitSuccessful) {
    if (ltr.newDataAvailable()) {
      retVal = ltr.readBothChannels(visPlusIrLevel, irLevel);
      // first meas allways low
      if (firstMeas) {
        firstMeas = false;
        retVal = false;
      }
    }
  } else { // force to day mode on boards with faulty sensor
    irLevel = 100;
    retVal = true;
  }
  return retVal;
}
#endif

void HardwareRev5::updateBacklightMode(uint16_t lightLevel) {
#ifdef OMOTE_KEYBRD_3661 // do we have a light sensor
  static bool backlight_mode_is_day = true;

  if (backlight_mode_is_day) { // hysteresis
    if (lightLevel < 16) {
      backlight_mode_is_day = false;
    }
  } else {
    if (lightLevel > 40) {
      backlight_mode_is_day = true;
    }
  }
  mDisplay->setDayMode(backlight_mode_is_day);
#endif
  HardwareRevX::updateBacklightMode(0);
}

struct keyState {
  unsigned long firstPressedTime = 0;
  unsigned long lastRepeatedTime = 0;
  bool isPressed = false;
  bool wasPressed = false;
  bool longSent = false;
};

bool HardwareRev5::keyboardScan() {
  static keyState keyStates[KEYPAD_ROWS * KEYPAD_COLS];
  bool keyPressed = false;
  bool keyEvent = false;
  uint8_t keyCode = 0;
  uint8_t row = 0, col = 0;
  uint8_t keyIndex = 0;
  uint8_t intStat = keypad.readRegister(TCA8418_REG_INT_STAT);
  if (intStat & 0x01) // Byte 0: K_INT (keyboard interrupt)
  {
    // datasheet page 16 - Table 2
    keyCode = keypad.getEvent();
    if (keyCode & 0x80)
      keyPressed = true;

    keyCode &= 0x7F;

    if (keyCode > 96) //  GPIO
    {
      keyCode -= 97;
// this only happens for key 'o' (off). Map this to vacant pos in matrix
#ifdef OMOTE_KEYBRD_3661
      row = 0;
      col = 5;
#else
      row = 1;
      col = 1;
#endif
    } else {
      // process matrix
      keyCode--;
      row = keyCode / 10;
      col = keyCode % 10;
      if ((row >= KEYPAD_ROWS) || (col >= KEYPAD_COLS))
        return false; // invalid key, should bever occur but don't process if
                      // it does
    }
    keyIndex = col + (row * KEYPAD_COLS);

    mLogger->setLogModule(LogModule::Keys);
    if (mLogger->isPrintWanted(LogLevel::Info)) {
      std::stringstream ss;
      ss << "Row:" << (uint16_t)row << ", Col:" << (uint16_t)col << ", Index:" << (uint16_t)keyIndex;
      mLogger->log(LogLevel::Info, ss);
    }

    //  clear the EVENT IRQ flag
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);

    // process
    if (keyPressed) { // new press so initialise structure
      keyStates[keyIndex].firstPressedTime = millis();
      keyStates[keyIndex].lastRepeatedTime =
          keyStates[keyIndex].firstPressedTime;
      keyStates[keyIndex].isPressed = true;
    } else {
      keyStates[keyIndex].isPressed = false;
    }
  }

  if (intStat & 0x02) // Byte 1: GPI_INT (GPIO interrupt)
  {
    //  reading the registers is mandatory to clear IRQ flag
    //  can also be used to find the GPIO changed
    //  as these registers are a bitmap of the gpio pins.
    keypad.readRegister(TCA8418_REG_GPIO_INT_STAT_1);
    keypad.readRegister(TCA8418_REG_GPIO_INT_STAT_2);
    keypad.readRegister(TCA8418_REG_GPIO_INT_STAT_3);
    //  clear GPIO IRQ flag
    keypad.writeRegister(TCA8418_REG_INT_STAT, 2);
  }

  //  check pending events
  // int intstat = keypad.readRegister(TCA8418_REG_INT_STAT); why, won't it just
  // loose events??

  // process keys
  KeyPressAbstract::KeyEvent event;
  BaseType_t higherPriorityTaskAwoke;
  unsigned long timeNow = millis();
  for (uint16_t index = 0; index < (KEYPAD_ROWS * KEYPAD_COLS); index++) {
    if (keyStates[index].isPressed != keyStates[index].wasPressed) {
      // change of state
      event.mId = Keys::CharKeyToKeyId(indexToChar[index]);
      if (keyStates[index].isPressed) {
        keyStates[index].longSent = false;
        event.mType = KeyPressAbstract::KeyEvent::Type::Press;
        mKeys->HandleKeyPresses(event);
        keyEvent = true;
        mLogger->debug("Press");
      } else {
        event.mType = KeyPressAbstract::KeyEvent::Type::Release;
        mKeys->HandleKeyPresses(event);
        mLogger->debug("Release");
        if (timeNow - keyStates[index].firstPressedTime < 500) {
          event.mType = KeyPressAbstract::KeyEvent::Type::Short;
          mKeys->HandleKeyPresses(event);
          keyEvent = true;
          mLogger->debug("Short");
        }
      }
      keyStates[index].wasPressed = keyStates[index].isPressed;
    } else {
      if (keyStates[index].isPressed) { // no change but still pressed
        if (timeNow - keyStates[index].lastRepeatedTime > 200) {
          // time to repeat
          keyStates[index].lastRepeatedTime = timeNow;
          event.mId = Keys::CharKeyToKeyId(indexToChar[index]);
          event.mType = KeyPressAbstract::KeyEvent::Type::Repeat;
          mKeys->HandleKeyPresses(event);
          keyEvent = true;
          mLogger->debug("Repeat");
        }
        if ((timeNow - keyStates[index].firstPressedTime >= 500) &&
            !keyStates[index].longSent) {
          // time to repeat
          keyStates[index].longSent = true;
          event.mId = Keys::CharKeyToKeyId(indexToChar[index]);
          event.mType = KeyPressAbstract::KeyEvent::Type::Long;
          mKeys->HandleKeyPresses(event);
          keyEvent = true;
          mLogger->debug("Long");
        }
      }
    }
  }
  return keyEvent;
}

void HardwareRev5::configIMUInterruptPolarity() {
  mIMU.writeRegister(LIS3DH_CTRL_REG6, 0x02); // For active-low interrupt
}

void HardwareRev5::enableWakeupByPin() {
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
}

void HardwareRev5::sleepDisplayPins() {
  pinMode(LCD_WR, INPUT_PULLDOWN);
  pinMode(LCD_RD, INPUT_PULLDOWN);
  pinMode(LCD_D0, INPUT_PULLDOWN);
  pinMode(LCD_D1, INPUT_PULLDOWN);
  pinMode(LCD_D2, INPUT_PULLDOWN);
  pinMode(LCD_D3, INPUT_PULLDOWN);
  pinMode(LCD_D4, INPUT_PULLDOWN);
  pinMode(LCD_D5, INPUT_PULLDOWN);
  pinMode(LCD_D6, INPUT_PULLDOWN);
  pinMode(LCD_D7, INPUT_PULLDOWN);

  pinMode(SD_EN, INPUT_PULLUP);
  pinMode(SD_CS, INPUT_PULLDOWN);
  pinMode(SD_MISO, INPUT_PULLDOWN);
  pinMode(SD_MOSI, INPUT_PULLDOWN);
  pinMode(SD_SCK, INPUT_PULLDOWN);
}

/*void HardwareRev5::sleepDisplayPins() {
  pinMode(LCD_WR, OUTPUT);
  digitalWrite(LCD_WR, LOW);
  pinMode(LCD_RD, OUTPUT);
  digitalWrite(LCD_RD, LOW);
  pinMode(LCD_D0, OUTPUT);
  digitalWrite(LCD_D0, LOW);
  pinMode(LCD_D1, OUTPUT);
  digitalWrite(LCD_D1, LOW);
  pinMode(LCD_D2, OUTPUT);
  digitalWrite(LCD_D2, LOW);
  pinMode(LCD_D3, OUTPUT);
  digitalWrite(LCD_D3, LOW);
  pinMode(LCD_D4, OUTPUT);
  digitalWrite(LCD_D4, LOW);
  pinMode(LCD_D5, OUTPUT);
  digitalWrite(LCD_D5, LOW);
  pinMode(LCD_D6, OUTPUT);
  digitalWrite(LCD_D6, LOW);
  pinMode(LCD_D7, OUTPUT);
  digitalWrite(LCD_D7, LOW);

  pinMode(SD_CS, OUTPUT);
  digitalWrite(SD_CS, LOW);
  pinMode(SD_MISO, OUTPUT);
  digitalWrite(SD_MISO, LOW);
  pinMode(SD_MOSI, OUTPUT);
  digitalWrite(SD_MOSI, LOW);
  pinMode(SD_SCK, OUTPUT);
  digitalWrite(SD_SCK, LOW);
}*/
