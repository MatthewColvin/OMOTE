#pragma once

// Comment out to disable connected features
#define ENABLE_WIFI
#define MQTT_SERVER "YOUR_MQTT_SERVER_IP"
#define MQTT_CLIENT_NAME "MQTT Client Name"

// time until device enters sleep mode in milliseconds
#define SLEEP_TIMEOUT 20000

// time until device remains in light sleep in milliseconds
#define LIGHT_SLEEP_TIMEOUT (1 * 60 * 1000)

// motion above threshold keeps device awake
#define MOTION_THRESHOLD 50

#if (defined(OMOTE_HARDWARE_REV5))
#include "Rev5PinDefs.h"
#else
#include "Rev1PinDefs.h"
#endif

// #define all GPIO accesses that aren't obvious without referring to schematic
#define LCD_EN_ON digitalWrite(LCD_EN, LOW)
#define LCD_EN_OFF digitalWrite(LCD_EN, HIGH)
#define IR_VCC_ON digitalWrite(IR_VCC, HIGH)
#define IR_VCC_OFF digitalWrite(IR_VCC, LOW)
#define SD_EN_ON digitalWrite(SD_EN, LOW)
#define SD_EN_OFF digitalWrite(SD_EN, HIGH)