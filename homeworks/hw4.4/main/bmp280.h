#pragma once

#include "esp_err.h"

/* BMP280 registers (datasheet 4.3). Unlike I2C there is no device address:
the chip is selected by pulling CSB low, and the first byte of every frame is
the register number — bit 7 set means read, cleared means write. */
#define BMP280_REG_CALIB 0x88 /* 0x88...0x9F, 24 bytes: T1...T3, P1...P9 */
#define BMP280_REG_ID 0xD0    /* stands in for WHO_AM_I: 0x58 */
#define BMP280_REG_RESET 0xE0 /* write 0xB6 — soft reset */
#define BMP280_REG_CTRL_MEAS 0xF4
#define BMP280_REG_CONFIG 0xF5
#define BMP280_REG_DATA 0xF7 /* 0xF7...0xFC, 6 bytes: pressure, temperature */

#define BMP280_CHIP_ID 0x58 /* 0x60 would be a BME280 — same bus, plus humidity */
#define BMP280_RESET_WORD 0xB6

typedef struct {
  float temperature; /* degrees Celsius */
  float pressure;    /* hPa */
} bmp280_data_t;

/* Brings up the SPI bus, checks the chip id, resets the sensor, reads its
factory calibration and leaves it measuring continuously (normal mode). */
esp_err_t bmp280_init(void);

/* One burst read of 0xF7...0xFC, so both channels come from the same sample. */
esp_err_t bmp280_read(bmp280_data_t* out);
