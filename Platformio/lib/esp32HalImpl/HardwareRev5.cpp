#include "HardwareRev5.hpp"

void HardwareRev5::init() {
  HardwareRev1::init();

  static constexpr auto MaxQueueableKeyPresses = 5;
  mKeysQueueHandle =
      xQueueCreate(MaxQueueableKeyPresses, sizeof(KeyPressAbstract::KeyEvent));

  mKeys = std::make_shared<Keys>(mKeysQueueHandle);
  setupKeyboard();

  Serial.printf("Finished Rev5 Hardware Init in %dms\r\n", millis());
}

void HardwareRev5::setupKeyboard() {
  if (!keypad.begin(TCA8418_DEFAULT_ADDR, &Wire)) {
    Serial.println("Keypad TCA8418 not found!");
  }
  keypad.matrix(KEYPAD_ROWS, KEYPAD_COLS);
  keypad.pinMode(5, INPUT_PULLUP);  // SW_PWR
  keypad.pinMode(6, INPUT_PULLUP);  // SD_DET
#ifdef OMOTE_KEYBRD_3661
  keypad.pinMode(14, INPUT);  // USB_3V3
#else
  keypad.pinMode(13, INPUT);  // USB_3V3
#endif

  pinMode(TCA_INT, INPUT);
  keypad.flush();
  keypad.writeRegister(TCA8418_REG_CFG, 0b00000001);
  keypad.writeRegister(TCA8418_REG_GPI_EM_1, KEYPAD_ROWS_BITMASK);
  keypad.writeRegister(TCA8418_REG_GPI_EM_2, KEYPAD_COLS_BITMASK);
}

void HardwareRev5::keyboardScan() {
  // std::array<keyPressDataStruct, KEYPAD_ROWS * KEYPAD_COLS>
  // keyPressData =
  // {0,0,KEY_IDLE};

  uint8_t keyCode = 0;
  uint8_t row = 0, col = 0;
  uint8_t keyIndex = 0;
  // keyStateEnum keyState = KEY_IDLE;
  bool keyPressed = false;
  uint8_t intStat = keypad.readRegister(TCA8418_REG_INT_STAT);
  if (intStat & 0x01)  // Byte 0: K_INT (keyboard interrupt)
  {
    // datasheet page 16 - Table 2
    keyCode = keypad.getEvent();
    if (keyCode & 0x80) keyPressed = true;

    keyCode &= 0x7F;

    if (keyCode > 96)  //  GPIO
    {
      keyCode -= 97;
#ifdef OMOTE_KEYBRD_3661
      // this only happens for key 'o' (off). Map this to 0/5
      row = 0;
      col = 5;
#else
      // this only happens for key 'o' (off). Map this to 1/1
      row = 1;
      col = 1;
#endif
      Serial.println(keyCode);
    } else {
      // process matrix
      keyCode--;
      row = keyCode / 10;
      col = keyCode % 10;
    }
    keyIndex = col + (row * KEYPAD_COLS);
    Serial.printf("Row:%d, Col %d, Index:%d\r\n", row, col, keyIndex);

    //  clear the EVENT IRQ flag
    keypad.writeRegister(TCA8418_REG_INT_STAT, 1);

    BaseType_t higherPriorityTaskAwoke;
    KeyPressAbstract::KeyEvent event =
        Keys::CharKeyToKeyId(indexToChar[keyIndex], keyPressed);
    xQueueSendFromISR(mKeysQueueHandle, &event, &higherPriorityTaskAwoke);
  }

  if (intStat & 0x02)  // Byte 1: GPI_INT (GPIO interrupt)
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
}

void HardwareRev5::configIMUInterruptPolarity() {
  IMU.writeRegister(LIS3DH_CTRL_REG6, 0x02);  // For active-low interrupt
}