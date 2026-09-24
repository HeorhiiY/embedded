/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdbool.h>
#include <stdint.h>
#include <stddef.h>
#include "driver/gpio.h"
#include "esp_timer.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "2.3-pio";

typedef struct {
  gpio_num_t pin;
  uint32_t period_ms;
  uint32_t last_ms;
  bool on;
} soft_led_t;

static soft_led_t leds[] = {
    {GPIO_NUM_4, 200, 0, false},
    {GPIO_NUM_7, 500, 0, false},
    {GPIO_NUM_8, 1000, 0, false},
};

static const size_t LED_COUNT = sizeof(leds) / sizeof(leds[0]);

static inline uint32_t millis_u32(void) {
  return (uint32_t)(esp_timer_get_time() / 1000ULL);
}

void app_main(void) {
  for (size_t i = 0; i < LED_COUNT; i++) {
    gpio_reset_pin(leds[i].pin);
    gpio_set_direction(leds[i].pin, GPIO_MODE_OUTPUT);
    gpio_set_level(leds[i].pin, leds[i].period_ms == 0 ? 1 : 0);
  }
  ESP_LOGI(TAG, "=== 2.3 PlatformIO ESP-IDF: 3 LED non-blocking ===");
  while (1) {
    const uint32_t now = millis_u32();
    for (size_t i = 0; i < LED_COUNT; i++) {
      soft_led_t* L = &leds[i];
      if (L->period_ms == 0) {
        gpio_set_level(L->pin, 1);
        continue;
      }
      if ((now - L->last_ms) >= L->period_ms) {
        L->last_ms = now;
        L->on = !L->on;
        gpio_set_level(L->pin, L->on ? 1 : 0);
      }
    }
    vTaskDelay(1);
  }
}
