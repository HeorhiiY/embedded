#include <stdint.h>
#include <string.h>
#include "config.h"
#include "driver/i2c_master.h"
#include "esp_err.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"

static const char* TAG = "4.3-i2c";
static i2c_master_bus_handle_t s_bus;

static void bus_init(void) {
  i2c_master_bus_config_t cfg = {
      .i2c_port = I2C_PORT,
      .sda_io_num = I2C_SDA_GPIO,
      .scl_io_num = I2C_SCL_GPIO,
      .clk_source = I2C_CLK_SRC_DEFAULT,
      .glitch_ignore_cnt = 7,
      /* Internal pull-up is a safety net. Rp is on the modules; not a substitute. */
      .flags.enable_internal_pullup = true,
  };

  ESP_ERROR_CHECK(i2c_new_master_bus(&cfg, &s_bus));
  ESP_LOGI(TAG, "bus SDA=GPIO%d SCL=GPIO%d %d Hz", (int)I2C_SDA_GPIO, (int)I2C_SCL_GPIO, I2C_HZ);
}

/* add_device never touches the bus: the address comes from the silkscreen, not a scan. */
static i2c_master_dev_handle_t attach(uint8_t addr) {
  i2c_device_config_t cfg = {
      .dev_addr_length = I2C_ADDR_BIT_LEN_7, /* datasheet 0x3C, not 0x78 */
      .device_address = addr,
      .scl_speed_hz = I2C_HZ,
  };

  i2c_master_dev_handle_t dev = NULL;
  ESP_ERROR_CHECK(i2c_master_bus_add_device(s_bus, &cfg, &dev));
  return dev;
}

/*
 * One packet: Co=0, D/C=0, then SSD1306 commands.
 * 0x8D 0x14 — charge pump (3.3 V). 0xDA 0x12 — COM pins for 128x64.
 */
static esp_err_t oled_setup(i2c_master_dev_handle_t dev, uint8_t addr) {
  static const uint8_t cmd[] = {
      0x00,       /* control byte: everything that follows is a command */
      0xAE,       /* Display OFF */
      0xD5, 0x80, /* clock frequency / divider */
      0xA8, 0x3F, /* multiplex 64 rows */
      0xD3, 0x00, /* display offset */
      0x40,       /* RAM start line */
      0x8D, 0x14, /* charge pump ON — mandatory at 3.3 V */
      0xA1,       /* segment remap */
      0xC8,       /* COM scan direction */
      0xDA, 0x12, /* COM pins for 128x64 */
      0x81, 0x7F, /* contrast */
      0xA6,       /* normal polarity */
      0xAF,       /* Display ON */
  };

  esp_err_t err = i2c_master_transmit(dev, cmd, sizeof(cmd), I2C_TIMEOUT_MS);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "OLED 0x%02X: setup rejected (%s)", addr, esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "OLED 0x%02X: configured, Display ON", addr);
  return ESP_OK;
}

/* Own packet, so the 0x00 control byte is repeated. 0xA5 lights every pixel, GDDRAM untouched. */
static esp_err_t oled_all_on(i2c_master_dev_handle_t dev, uint8_t addr) {
  static const uint8_t cmd[] = {
      0x00, /* control byte: everything that follows is a command */
      0xA5, /* Entire Display ON (0xA4 resumes from RAM) */
  };

  esp_err_t err = i2c_master_transmit(dev, cmd, sizeof(cmd), I2C_TIMEOUT_MS);
  if (err != ESP_OK) {
    ESP_LOGE(TAG, "OLED 0x%02X: 0xA5 rejected (%s)", addr, esp_err_to_name(err));
    return err;
  }

  ESP_LOGI(TAG, "OLED 0x%02X: all pixels on (0xA5)", addr);
  return ESP_OK;
}

static esp_err_t oled_cmds(i2c_master_dev_handle_t dev, const uint8_t* c, size_t n) {
  uint8_t buf[16];
  if (n + 1 > sizeof(buf))
    return ESP_ERR_INVALID_SIZE;
  buf[0] = 0x00; /* control byte: commands */
  memcpy(&buf[1], c, n);
  return i2c_master_transmit(dev, buf, n + 1, I2C_TIMEOUT_MS);
}

static esp_err_t oled_breathe(i2c_master_dev_handle_t dev, int cycles) {
  uint8_t cmd[2];
  cmd[0] = 0x81; /* Set Contrast */

  for (int k = 0; k < cycles; k++) {
    /* up: 0 → 255 */
    for (int c = 0; c <= 255; c += 5) {
      cmd[1] = (uint8_t)c;
      esp_err_t err = oled_cmds(dev, cmd, 2);
      if (err != ESP_OK)
        return err;
      vTaskDelay(pdMS_TO_TICKS(10));
    }
    /* down: 255 → 0 */
    for (int c = 255; c >= 0; c -= 5) {
      cmd[1] = (uint8_t)c;
      esp_err_t err = oled_cmds(dev, cmd, 2);
      if (err != ESP_OK)
        return err;
      vTaskDelay(pdMS_TO_TICKS(10));
    }
  }
  return ESP_OK;
}

static esp_err_t oled_start(i2c_master_dev_handle_t dev, uint8_t addr) {
  for (int attempt = 1; attempt <= 3; attempt++) {
    /* 9 SCL pulses + STOP: a slave stuck mid-byte releases SDA */
    i2c_master_bus_reset(s_bus);

    esp_err_t p = i2c_master_probe(s_bus, addr, I2C_TIMEOUT_MS);
    ESP_LOGI(TAG, "attempt %d: probe %s", attempt, esp_err_to_name(p));

    if (oled_setup(dev, addr) == ESP_OK) {
      return ESP_OK;
    }
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  return ESP_FAIL;
}

void app_main(void) {
  bus_init();
  vTaskDelay(pdMS_TO_TICKS(1000)); // gives some time for the bus to settle
  i2c_master_dev_handle_t oled = attach(ADDR_OLED);

  /* Charge pump and Display ON come from setup — 0xA5 alone shows nothing. */
  if (oled_start(oled, ADDR_OLED) == ESP_OK) {
    oled_all_on(oled, ADDR_OLED);

    /* One cycle per pass, so a NACK mid-fade stops the loop instead of spinning. */
    for (;;) {
      esp_err_t err = oled_breathe(oled, 1);
      if (err != ESP_OK) {
        ESP_LOGE(TAG, "OLED 0x%02X: breathe stopped (%s)", ADDR_OLED, esp_err_to_name(err));
        break;
      }
    }
  }
}
