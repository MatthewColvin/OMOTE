#pragma once

// Comment out to disable connected features
#define ENABLE_WIFI
#define MQTT_SERVER "YOUR_MQTT_SERVER_IP"
#define MQTT_CLIENT_NAME "MQTT Client Name"

// time until device enters sleep mode in milliseconds
#define SLEEP_TIMEOUT 20000

// motion above threshold keeps device awake
#define MOTION_THRESHOLD 50

#if defined(OMOTE_HARDWARE_REV5)
  #if defined(OMOTE_KEYBRD_3661)
    #define KEYPAD_ROWS 5  // five rows
    #define KEYPAD_COLS 6  // six columns

    #define LCD_DC 20  // defined in TFT_eSPI User_Setup.h
    #define LCD_CS 21
    #define LCD_BL 41
    #define LCD_EN 9
    #define LCD_WR 19
    #define LCD_RD 18
    #define LCD_D0 17
    #define LCD_D1 16
    #define LCD_D2 15
    #define LCD_D3 14
    #define LCD_D4 13
    #define LCD_D5 12
    #define LCD_D6 11
    #define LCD_D7 10

    #define USER_LED 45

    #define IR_RX 6    // IR receiver input
    #define IR_VCC 7   // IR receiver power
    #define IR_LED 46  // IR LED output

    #define SCL 2
    #define SDA 1

    #define CRG_STAT 8

    #define ACC_INT 5
    #define TCA_INT 4

    #define SD_EN 47
    #define SD_CS 40
    #define SD_MISO 48
    #define SD_MOSI 39
    #define SD_SCK 38

    #define KBD_BL 42

    #define LCD_BL_OFF digitalWrite(LCD_BL, LOW)
    #define KBD_BL_OFF digitalWrite(KBD_BL, LOW)
  #else
    #define KEYPAD_ROWS 5  // five rows
    #define KEYPAD_COLS 5  // five columns

    #define LCD_DC 40  // defined in TFT_eSPI User_Setup.h
    #define LCD_CS 39
    #define LCD_BL 9
    #define LCD_EN 38
    #define LCD_WR 41
    #define LCD_RD 42
    #define LCD_D0 48
    #define LCD_D1 47
    #define LCD_D2 21
    #define LCD_D3 14
    #define LCD_D4 13
    #define LCD_D5 12
    #define LCD_D6 11
    #define LCD_D7 10

    #define USER_LED 45

    #define IR_RX 4   // IR receiver input
    #define IR_VCC 6  // IR receiver power
    #define IR_LED 5  // IR LED output

    #define SCL 19
    #define SDA 20
    #define ACC_INT 2
    #define CRG_STAT 1
    #define TCA_INT_GPIO 8

    #define SD_EN_GPIO 16
    #define SD_CS_GPIO 18
    #define SD_MISO_GPIO 7
    #define SD_MOSI_GPIO 17
    #define SD_SCK_GPIO 15

    #define KBD_BL 46

    #define LCD_BL_OFF digitalWrite(LCD_BL, HIGH)

  #endif
#define BUTTON_PIN_BITMASK ((0x01 << TCA_INT) | (0x01 << ACC_INT))
#define KEYPAD_ROWS_BITMASK      \
  ((0x01 << (KEYPAD_ROWS + 1)) | \
   ((0x01 << (KEYPAD_ROWS + 1)) - 1))  // num rows plus PWR button
#define KEYPAD_COLS_BITMASK \
  ((0x01 << KEYPAD_COLS) |  \
   ((0x01 << KEYPAD_COLS) - 1))  // num cols only, no interrupt for USB_3V3
#define LEDC_SPEED_MODE LEDC_LOW_SPEED_MODE
#else

#define KEYPAD_ROWS 5  // five rows
#define KEYPAD_COLS 5  // five columns
// IO34+IO35+IO37+IO38+IO39(+IO13)
//#define BUTTON_PIN_BITMASK 0b1110110000000000000000000010000000000000

// Pin assignment
// -----------------------------------------------------------------------------------------------------------------------

#define LCD_DC 9  // defined in TFT_eSPI User_Setup.h
#define LCD_CS 5
#define LCD_MOSI 23
#define LCD_SCK 18
#define LCD_BL 4
#define LCD_EN 10

#define USER_LED 2

#define SW_1 32  // 1...5: Output
#define SW_2 26
#define SW_3 27
#define SW_4 14
#define SW_5 12
#define SW_A 37  // A...E: Input
#define SW_B 38
#define SW_C 39
#define SW_D 34
#define SW_E 35

#define IR_RX 15    // IR receiver input
#define ADC_BAT 36  // Battery voltage sense input (1/2 divider)
#define IR_VCC 25   // IR receiver power
#define IR_LED 33   // IR LED output

#define TFT_SCL 22
#define TFT_SDA 19
#define ACC_INT 20

#define CRG_STAT 21  // battery charger feedback

#define LCD_BL_OFF digitalWrite(LCD_BL, HIGH)

#define LEDC_SPEED_MODE LEDC_HIGH_SPEED_MODE

#define BUTTON_PIN_BITMASK                                             \
  ((0x01 << SW_A) | (0x01 << SW_B) | (0x01 << SW_C) | (0x01 << SW_D) | \
   (0x01 << SW_E) | (0X01 << ACC_INT))
#endif

// #define all GPIO accesses that aren't obvious without referring to schematic
#define LCD_EN_ON digitalWrite(LCD_EN, LOW)
#define LCD_EN_OFF digitalWrite(LCD_EN, HIGH)
#define IR_VCC_ON digitalWrite(IR_VCC, HIGH)
#define IR_VCC_OFF digitalWrite(IR_VCC, LOW)
#define SD_EN_ON digitalWrite(SD_EN, LOW)
#define SD_EN_OFF digitalWrite(SD_EN, HIGH)