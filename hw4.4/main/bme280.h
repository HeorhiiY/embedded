#pragma once

#include "esp_err.h"

/* BME280 registers (datasheet 5.3). Unlike I2C there is no device address:
the chip is selected by pulling CSB low, and the first byte of every frame is
the register number — bit 7 set means read, cleared means write. */
#define BME280_REG_CALIB_T_P 0x88 /* 0x88...0xA1, 26 bytes: T1...T3, P1...P9, H1 */
#define BME280_REG_ID 0xD0        /* stands in for WHO_AM_I: 0x60 */
#define BME280_REG_RESET 0xE0     /* write 0xB6 — soft reset */
#define BME280_REG_CALIB_H 0xE1   /* 0xE1...0xE7, 7 bytes: H2...H6 */
#define BME280_REG_CTRL_HUM 0xF2
#define BME280_REG_CTRL_MEAS 0xF4
#define BME280_REG_CONFIG 0xF5
#define BME280_REG_DATA 0xF7 /* 0xF7...0xFE, 8 bytes: pressure, temperature, humidity */

#define BME280_CHIP_ID 0x60 /* 0x58 would be a BMP280 — same bus, no humidity */
#define BME280_RESET_WORD 0xB6

typedef struct {
  float temperature; /* degrees Celsius */
  float pressure;    /* hPa */
  float humidity;    /* percent relative humidity */
} bme280_data_t;

/* Brings up the SPI bus, checks the chip id, resets the sensor, reads its
factory calibration and leaves it measuring continuously (normal mode). */
esp_err_t bme280_init(void);

/* One burst read of 0xF7...0xFE, so all three channels come from the same sample. */
esp_err_t bme280_read(bme280_data_t* out);
