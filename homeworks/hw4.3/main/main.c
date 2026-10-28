/*
 * 4.4: DS1307 clock on the OLED.
 * Every start writes START_TIME, so the count always begins at 09:00 18.10.2026.
 * Bus J1: GPIO8 SDA, GPIO9 SCL, 100 kHz.
 * Addresses are known: clock 0x68, display 0x3C (fallback 0x3D). No bus scan.
 */

#include <stdbool.h>
#include <stdio.h>
#include <string.h>
#include "config.h"
#include "driver/i2c_master.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "oled.h"
#include "rtc.h"

static const char* TAG = "4.4-rtc";
static i2c_master_bus_handle_t s_bus;

/* 09:00:00 18.10.2026. That date is a Sunday, hence wday = 1 (datasheet numbering). */
static const ds1307_time_t START_TIME = {
    .sec = 0, .min = 0, .hour = 9, .wday = 1, .day = 18, .mon = 10, .year = 2026};

static void bus_init(void) {
  i2c_master_bus_config_t cfg = {
      .i2c_port = I2C_PORT,
      .sda_io_num = I2C_SDA_GPIO,
      .scl_io_num = I2C_SCL_GPIO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_bus));
  ESP_LOGI(TAG, "bus SDA=GPIO%d SCL=GPIO%d %d Hz", (int)I2C_SDA_GPIO, (int)I2C_SCL_GPIO, I2C_HZ);
}

static i2c_master_dev_handle_t attach(uint8_t addr) {
  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7,
      .device_address = addr,
      .scl_speed_hz = I2C_HZ,
  };

  i2c_master_dev_handle_t dev = NULL;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(s_bus, &cfg, &dev));
  return dev;
}

/* Only the known pair of addresses, not a 0x08...0x77 scan. */
static i2c_master_dev_handle_t attach_known(uint8_t a, uint8_t alt, const char* name) {
  if (i2c_master_probe(s_bus, a, I2C_TIMEOUT_MS) == ESP_OK) {
    ESP_LOGI(TAG, "%s ACK 0x%02X", name, a);
    return attach(a);
  }

  if (alt != a && i2c_master_probe(s_bus, alt, I2C_TIMEOUT_MS) == ESP_OK) {
    ESP_LOGI(TAG, "%s ACK 0x%02X", name, alt);
    return attach(alt);
  }

  ESP_LOGW(TAG, "%s 0x%02X/0x%02X: no ACK", name, a, alt);
  return NULL;
}

static void fmt_date(char* out, size_t cap, const ds1307_time_t* t, bool ok) {
  static const char* WDAY[7] = {"SUN", "MON", "TUE", "WED", "THU", "FRI", "SAT"};
  if (!ok) {
    snprintf(out, cap, "----.--.-- ---");
    return;
  }

  snprintf(out, cap, "%04u-%02u-%02u %s", (unsigned)t->year, (unsigned)t->mon, (unsigned)t->day,
           WDAY[(t->wday ? t->wday - 1u : 0u) % 7u]);
}

static void fmt_time(char* out, size_t cap, const ds1307_time_t* t, bool ok) {
  if (!ok) {
    snprintf(out, cap, "--:--:--");
    return;
  }

  snprintf(out, cap, "%02u:%02u:%02u", (unsigned)t->hour, (unsigned)t->min, (unsigned)t->sec);
}

void app_main(void) {
  char dline[22];
  char tline[22];
  ds1307_time_t now;
  bool rtc_ok = false;
  int last_sec = -1;

  bus_init();
  i2c_master_dev_handle_t rtc = attach_known(ADDR_RTC, ADDR_RTC, "DS1307");
  i2c_master_dev_handle_t oled = attach_known(ADDR_OLED, ADDR_OLED_ALT, "OLED");

  memset(&now, 0, sizeof(now));
  if (rtc && ds1307_init(rtc) != ESP_OK) {
    ESP_LOGE(TAG, "DS1307: registers 0x00...0x06 do not look like time");
    rtc = NULL;
  }

  /* The time is written on every start, the previous count is not kept. */
  if (rtc && ds1307_set_time(rtc, &START_TIME) != ESP_OK) {
    ESP_LOGE(TAG, "DS1307: writing the time failed");
  }

  if (oled) {
    if (oled_init(oled) != ESP_OK) {
      ESP_LOGE(TAG, "OLED: init NACK");
      oled = NULL;
    }
  }

  fmt_date(dline, sizeof(dline), &now, false);
  fmt_time(tline, sizeof(tline), &now, false);
  if (oled) {
    oled_fb_clear();
    oled_text_xy(0, 0, dline);
    oled_text_xy(0, 8, tline);
    oled_flush();
  }

  for (;;) {
    /* Polled three times per second, so the display never skips a digit.
    While the second is unchanged: no log line and no frame pushed onto the bus. */
    vTaskDelay(pdMS_TO_TICKS(330));
    if (!rtc) {
      continue;
    }

    rtc_ok = ds1307_get_time(rtc, &now) == ESP_OK;
    if (!rtc_ok) {
      ESP_LOGE(TAG, "DS1307: read failed");
      last_sec = -1;
    } else if ((int)now.sec == last_sec) {
      continue;
    } else {
      last_sec = (int)now.sec;
    }

    fmt_date(dline, sizeof(dline), &now, rtc_ok);
    fmt_time(tline, sizeof(tline), &now, rtc_ok);
    if (rtc_ok) {
      ESP_LOGI(TAG, "%s %s", dline, tline);
    }
    if (oled) {
      oled_fb_clear();
      oled_text_xy(0, 0, dline);
      oled_text_xy(0, 8, tline);
      oled_flush();
    }
  }
}
