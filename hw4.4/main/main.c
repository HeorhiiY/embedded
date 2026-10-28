/*
 * 4.4: BME280 over SPI — temperature, pressure and humidity, once a second, into the log.
 * Bus: GPIO8 SCLK, GPIO9 MOSI (sensor SDI), GPIO10 MISO (sensor SDO), GPIO11 CS.
 * 1 MHz, mode 0. The sensor is the only device on the bus, so there is nothing to probe:
 * either register 0xD0 reads back 0x60 or the wiring is wrong.
 */

#include "bme280.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "4.4-bme280";

void app_main(void) {
  bme280_data_t data;

  if (bme280_init() != ESP_OK) {
    ESP_LOGE(TAG, "BME280 init failed, nothing to read");
    return;
  }

  for (;;) {
    vTaskDelay(pdMS_TO_TICKS(1000));

    if (bme280_read(&data) != ESP_OK) {
      ESP_LOGE(TAG, "read failed");
      continue;
    }

    ESP_LOGI(TAG, "T = %.2f C   P = %.2f hPa   H = %.2f %%", data.temperature, data.pressure,
             data.humidity);
  }
}
