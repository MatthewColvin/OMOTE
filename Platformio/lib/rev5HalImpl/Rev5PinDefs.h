#pragma once

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
#define ADC_BAT -1 //not usde, needed to keep common battery constructor

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
