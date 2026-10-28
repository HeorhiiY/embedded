#include "rtc.h"
#include <stdbool.h>
#include "config.h"
#include "esp_log.h"

static const char* TAG = "ds1307";

static uint8_t bcd2dec(uint8_t b) {
  return (uint8_t)((b >> 4) * 10u + (b & 0x0Fu));
}

static uint8_t dec2bcd(uint8_t d) {
  return (uint8_t)(((d / 10u) << 4) | (d % 10u));
}

/* Both nibbles must be digits: 0x1A is not BCD, even though it reads fine as a register. */
static bool bcd_in(uint8_t b, uint8_t lo, uint8_t hi) {
  uint8_t v;
  if ((b & 0x0Fu) > 9 || (b >> 4) > 9) {
    return false;
  }

  v = bcd2dec(b);
  return v >= lo && v <= hi;
}

esp_err_t ds1307_read(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t* buf, size_t n) {
  /* Read frame: ADDR+W · REG · Sr · ADDR+R · DATA... · NACK bit · STOP.
  The address counter inside the DS1307 advances by itself, so a burst is one frame. */
  return i2c_master_transmit_receive(dev, &reg, 1, buf, n, I2C_TIMEOUT_MS);
}

esp_err_t ds1307_write_u8(i2c_master_dev_handle_t dev, uint8_t reg, uint8_t val) {
  /* Write: ADDR+W · REG · DATA · STOP — no repeated START. */
  uint8_t pkt[2] = {reg, val};
  return i2c_master_transmit(dev, pkt, sizeof(pkt), I2C_TIMEOUT_MS);
}

/* Hours are either 24-hour (0...23) or 12-hour (1...12 plus the PM bit). */
static uint8_t hour_to_24(uint8_t reg) {
  uint8_t h;
  if ((reg & DS1307_12H) == 0) {
    return bcd2dec((uint8_t)(reg & 0x3Fu));
  }

  h = bcd2dec((uint8_t)(reg & 0x1Fu));
  if (reg & DS1307_PM) {
    return h < 12 ? (uint8_t)(h + 12) : h;
  }
  return h == 12 ? 0 : h;
}

static bool regs_look_like_time(const uint8_t* r) {
  bool hour_ok = (r[DS1307_REG_HOUR] & DS1307_12H)
                     ? bcd_in((uint8_t)(r[DS1307_REG_HOUR] & 0x1Fu), 1, 12)
                     : bcd_in((uint8_t)(r[DS1307_REG_HOUR] & 0x3Fu), 0, 23);

  return bcd_in((uint8_t)(r[DS1307_REG_SEC] & 0x7Fu), 0, 59) && bcd_in(r[DS1307_REG_MIN], 0, 59) &&
         hour_ok && bcd_in(r[DS1307_REG_WDAY], 1, 7) && bcd_in(r[DS1307_REG_DAY], 1, 31) &&
         bcd_in(r[DS1307_REG_MON], 1, 12) && bcd_in(r[DS1307_REG_YEAR], 0, 99);
}

esp_err_t ds1307_init(i2c_master_dev_handle_t dev) {
  uint8_t r[7];
  uint8_t sec;
  bool ch;
  esp_err_t err = ds1307_read(dev, DS1307_REG_SEC, r, sizeof(r));
  if (err != ESP_OK) {
    return err;
  }

  /* No WHO_AM_I: the DS1307 signature is valid BCD in all seven fields. */
  if (!regs_look_like_time(r)) {
    ESP_LOGE(TAG, "0x00...0x06 is not BCD time (%02X %02X %02X %02X %02X %02X %02X)", r[0], r[1],
             r[2], r[3], r[4], r[5], r[6]);
    return ESP_ERR_NOT_FOUND;
  }

  sec = r[DS1307_REG_SEC];
  ch = (sec & DS1307_CH) != 0;

  /* 12 → 24 once at startup: past this point the driver only reads and writes 24-hour. */
  if (r[DS1307_REG_HOUR] & DS1307_12H) {
    err = ds1307_write_u8(dev, DS1307_REG_HOUR, dec2bcd(hour_to_24(r[DS1307_REG_HOUR])));
    if (err != ESP_OK) {
      return err;
    }
  }

  /* CH=0 starts the oscillator; the seconds keep the value already in the register. */
  if (ch) {
    err = ds1307_write_u8(dev, DS1307_REG_SEC, (uint8_t)(sec & 0x7Fu));
    if (err != ESP_OK) {
      return err;
    }
  }

  /* 0x07: OUT=0, SQWE=0 — hold the SQW/OUT pin low, no square wave needed. */
  err = ds1307_write_u8(dev, DS1307_REG_CTRL, 0x00);
  if (err != ESP_OK) {
    return err;
  }

  ESP_LOGI(TAG, "DS1307 at 0x%02X, SQW off, %s", (int)ADDR_RTC,
           ch ? "CH was 1 — time lost" : "clock was running");
  return ESP_OK;
}

esp_err_t ds1307_get_time(i2c_master_dev_handle_t dev, ds1307_time_t* t) {
  uint8_t r[7];
  esp_err_t err = ds1307_read(dev, DS1307_REG_SEC, r, sizeof(r));
  if (err != ESP_OK) {
    return err;
  }

  if (!regs_look_like_time(r)) {
    return ESP_ERR_INVALID_RESPONSE;
  }

  t->sec = bcd2dec((uint8_t)(r[DS1307_REG_SEC] & 0x7Fu));
  t->min = bcd2dec(r[DS1307_REG_MIN]);
  t->hour = hour_to_24(r[DS1307_REG_HOUR]);
  t->wday = bcd2dec(r[DS1307_REG_WDAY]);
  t->day = bcd2dec(r[DS1307_REG_DAY]);
  t->mon = bcd2dec(r[DS1307_REG_MON]);
  t->year = (uint16_t)(2000u + bcd2dec(r[DS1307_REG_YEAR]));
  return ESP_OK;
}

esp_err_t ds1307_set_time(i2c_master_dev_handle_t dev, const ds1307_time_t* t) {
  uint8_t pkt[8];
  if (t->sec > 59 || t->min > 59 || t->hour > 23 || t->wday < 1 || t->wday > 7 || t->day < 1 ||
      t->day > 31 || t->mon < 1 || t->mon > 12 || t->year < 2000 || t->year > 2099) {
    return ESP_ERR_INVALID_ARG;
  }

  pkt[0] = DS1307_REG_SEC;
  pkt[1] = dec2bcd(t->sec); /* CH=0: writing the seconds is also the start */
  pkt[2] = dec2bcd(t->min);
  pkt[3] = dec2bcd(t->hour); /* bit 6 = 0 — 24-hour format */
  pkt[4] = dec2bcd(t->wday);
  pkt[5] = dec2bcd(t->day);
  pkt[6] = dec2bcd(t->mon);
  pkt[7] = dec2bcd((uint8_t)(t->year - 2000u));
  return i2c_master_transmit(dev, pkt, sizeof(pkt), I2C_TIMEOUT_MS);
}
