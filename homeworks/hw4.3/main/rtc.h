#pragma once

#include <stddef.h>
#include <stdint.h>
#include "driver/i2c_master.h"
#include "esp_err.h"

/* DS1307: address 0x68 is fixed in silicon, there is no WHO_AM_I register.
0x00...0x06 — time in BCD, 0x07 — SQW/OUT control, 0x08...0x3F — 56 bytes of NVRAM. */
#define DS1307_REG_SEC 0x00
#define DS1307_REG_MIN 0x01
#define DS1307_REG_HOUR 0x02
#define DS1307_REG_WDAY 0x03
#define DS1307_REG_DAY 0x04
#define DS1307_REG_MON 0x05
#define DS1307_REG_YEAR 0x06
#define DS1307_REG_CTRL 0x07
#define DS1307_REG_NVRAM 0x08
#define DS1307_NVRAM_SIZE 56

#define DS1307_CH 0x80  /* 0x00 bit 7: 1 — oscillator stopped */
#define DS1307_12H 0x40 /* 0x02 bit 6: 1 — 12-hour format */
#define DS1307_PM 0x20  /* 0x02 bit 5: PM, in 12-hour mode only */

typedef struct {
  uint8_t sec, min, hour; /* 0...59, 0...59, 0...23 — always 24-hour */
  uint8_t wday;           /* 1...7, kept consistent by the caller (1 = Sunday) */
  uint8_t day, mon;       /* 1...31, 1...12 */
  uint16_t year;          /* 2000...2099 */
} ds1307_time_t;

esp_err_t ds1307_read(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t* buf, size_t n);
esp_err_t ds1307_write_u8(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val);

/* Stands in for WHO_AM_I: checks that 0x00...0x06 look like BCD time.
Puts the chip into 24-hour mode, silences SQW/OUT and clears CH.
Whether CH was set before the start goes to the log, not back as a flag. */
esp_err_t ds1307_init(i2c_master_dev_handle_t dev);

/* Burst 0x00...0x06 in one frame: seconds and date from the same tick. */
esp_err_t ds1307_get_time(i2c_master_dev_handle_t dev, ds1307_time_t* t);

/* Writes 0x00...0x06 in one frame; CH=0 in the seconds also starts the oscillator. */
esp_err_t ds1307_set_time(i2c_master_dev_handle_t dev, const ds1307_time_t* t);
