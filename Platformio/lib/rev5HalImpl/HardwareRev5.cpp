#include "HardwareRev5.hpp"
#include <Adafruit_TCA8418.h>
#include <LittleFS.h>

HardwareRev5::HardwareRev5() : mLogger(std::make_unique<LoggingInterface>()) {
  mLogger->setLogModule(LogModule::General);
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

  static constexpr auto MaxQueueableKeyPresses = 5;

  mKeys = std::make_shared<Keys>();
  setupKeyboard();

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

void HardwareRev5::initIO() {
  HardwareRevX::initIO();
  SD_EN_OFF;
  KBD_BL_OFF;
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
  keypad.flush();
  keypad.writeRegister(TCA8418_REG_CFG, 0b00000001);
  keypad.writeRegister(TCA8418_REG_GPI_EM_1, KEYPAD_ROWS_BITMASK);
  keypad.writeRegister(TCA8418_REG_GPI_EM_2, KEYPAD_COLS_BITMASK);
}

#ifdef OMOTE_KEYBRD_3661
void HardwareRev5::setupLightSensor() {
  if (ltr.begin()) {
    ltr.setGain(LTR3XX_GAIN_8);
    ltr.setIntegrationTime(LTR3XX_INTEGTIME_100);
    ltr.setMeasurementRate(LTR3XX_MEASRATE_100);
    mlightSensorInitSuccessful = true;
  } else {
    mLogger->setLogModule(LogModule::Display);
    if (mLogger->isPrintWanted(LogLevel::Error))
      mLogger->log(LogLevel::Error, "Couldn't find LTR-303 sensor!");
  }
}

bool HardwareRev5::lightSensorScan(uint16_t &visPlusIrLevel,
                                   uint16_t &irLevel) {
  bool retVal = false;
  if (mlightSensorInitSuccessful) {
    if (ltr.newDataAvailable())
      retVal = ltr.readBothChannels(visPlusIrLevel, irLevel);
  }
  return retVal;
}
#endif

void HardwareRev5::updateBacklightMode(uint16_t lightLevel) {
#ifdef OMOTE_KEYBRD_3661 // do we have a light sensor
  static bool backlight_mode_is_day = true;
  static bool firstMeas = true;

  if (firstMeas) {
    firstMeas = false;
    return;
  }

  if (backlight_mode_is_day) { // hysteresis
    if (lightLevel < 20) {
      backlight_mode_is_day = false;
      mDisplay->setDayMode(backlight_mode_is_day);
    }
  } else {
    if (lightLevel > 60) {
      backlight_mode_is_day = true;
      mDisplay->setDayMode(backlight_mode_is_day);
    }
  }
#endif
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
        mLogger->debug("Press");
      } else {
        event.mType = KeyPressAbstract::KeyEvent::Type::Release;
        mKeys->HandleKeyPresses(event);
        mLogger->debug("Release");
        if (timeNow - keyStates[index].firstPressedTime < 500) {
          event.mType = KeyPressAbstract::KeyEvent::Type::Short;
          mKeys->HandleKeyPresses(event);
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
          mLogger->debug("Repeat");
        }
        if ((timeNow - keyStates[index].firstPressedTime >= 500) &&
            !keyStates[index].longSent) {
          // time to repeat
          keyStates[index].longSent = true;
          event.mId = Keys::CharKeyToKeyId(indexToChar[index]);
          event.mType = KeyPressAbstract::KeyEvent::Type::Long;
          mKeys->HandleKeyPresses(event);
          mLogger->debug("Long");
        }
      }
    }
  }
  return keyPressed;
}

void HardwareRev5::configIMUInterruptPolarity() {
  mIMU.writeRegister(LIS3DH_CTRL_REG6, 0x02); // For active-low interrupt
}

void HardwareRev5::enableWakeupByPin() {
  esp_sleep_enable_ext1_wakeup(BUTTON_PIN_BITMASK, ESP_EXT1_WAKEUP_ANY_LOW);
}

void HardwareRev5::sleepDisplayPins() {
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
}
