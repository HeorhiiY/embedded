#pragma once
#include "driver/i2c_master.h"
/* J1 on the ESP32-S3-N16R8: the same row as 3V3 and GND.
Between GPIO8 and GPIO9 on the header sit 3 and 46 — do not take those. */
#define I2C_PORT I2C_NUM_0
#define I2C_SDA_GPIO GPIO_NUM_8
#define I2C_SCL_GPIO GPIO_NUM_9
#define I2C_HZ 100000
#define I2C_TIMEOUT_MS 200
/* Seven-bit addresses as in the datasheet. ESP-IDF does not shift them << 1 (unlike HAL on F4).
No bus scan: only this pair gets probed. */
#define ADDR_OLED 0x3C /* SSD1306, SA0 = 0 */
#define ADDR_OLED_ALT 0x3D
#define ADDR_RTC 0x68 /* DS1307, address fixed in silicon — no alternative */
