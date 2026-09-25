/*
 * SPDX-FileCopyrightText: 2010-2022 Espressif Systems (Shanghai) CO LTD
 *
 * SPDX-License-Identifier: CC0-1.0
 */

#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "esp_intr_alloc.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

#include "config.h"

static const char* TAG = "2.4";

/* ever mode below has its own handler defined */
static void IRAM_ATTR gpio_isr_handler(void* arg);

// millis function for simplicity
static inline uint32_t millis(void) {
  return (uint32_t)(esp_timer_get_time() / 1000);
}

// the version without gpio_config, direct usage of the functions
static void setup(void) {
  gpio_reset_pin(BTN_GPIO);
  gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);
  gpio_set_intr_type(BTN_GPIO, GPIO_INTR_NEGEDGE);

  gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  gpio_isr_handler_add(BTN_GPIO, gpio_isr_handler, NULL);
  gpio_intr_enable(BTN_GPIO);
}

/* ======================= MODE 1 ======================= */
/* Рахувати всі фронти: без debounce, без delay */
#if MODE == 1

static volatile uint32_t press_count = 0;
static uint32_t last_count = 0;

static void gpio_isr_handler(void* arg) {
  (void)arg;
  press_count++;
}

void app_main(void) {
  setup();

  while (1) {
    if (press_count != last_count) {
      ESP_LOGI(TAG, "press count=%lu", (unsigned long)press_count);
      last_count = press_count;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
}

/* ======================= MODE 2 ======================= */
/* Debounce по мітці часу: ігнорувати фронти в межах вікна */
#elif MODE == 2

static volatile uint32_t edges_count = 0;
static uint32_t last_count = 0;
static uint32_t last_press_time = 0;
static uint32_t press_count = 0;

static void gpio_isr_handler(void* arg) {
  (void)arg;
  edges_count++;
}

void app_main(void) {
  setup();
  uint32_t now;
  while (1) {
    now = millis();
    if (edges_count != last_count) {
      if (now - last_press_time >= 50) {
        press_count++;
        ESP_LOGI(TAG, "press count=%lu", (unsigned long)press_count);
        last_press_time = now;
      }
      else{
        ESP_LOGI(TAG, "bounce ignored");
      }
      last_count = edges_count;
    }
    vTaskDelay(1);//pdMS_TO_TICKS(10));
  }
}

/* ======================= MODE 3 ======================= */
/* Debounce with a flag, but with blocks for DEBOUNSE_MS */
#elif MODE == 3

static volatile bool btn_irq_pending = false; // flag for ISR
static uint32_t press_count = 0;

static void gpio_isr_handler(void* arg) {
  (void)arg;
  btn_irq_pending = true;
}

void handle_button(){
  if (!btn_irq_pending) {
    return;
  }

  if (btn_irq_pending) {
    vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
      if (gpio_get_level(BTN_GPIO) != 0) {
        btn_irq_pending = false;
        return;
      }

      press_count++;
      btn_irq_pending = false;
      ESP_LOGI(TAG, "press count=%lu", (unsigned long)press_count);
  }
}

void app_main(void) {
  setup();

  while (1) {
    handle_button();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}

#else

static volatile bool btn_irq_pending = false; // flag for ISR
static uint32_t pending_time = 0;
static uint32_t press_count = 0;

static void gpio_isr_handler(void* arg) {
  (void)arg;
  btn_irq_pending = true;
}

void handle_button(uint32_t now){
  if (!btn_irq_pending) {
    return;
  }

  pending_time = now;
  btn_irq_pending = false;
  return;
}

void request_toggle(uint32_t now){
  if (pending_time == 0) {
    return;
  }
  if (now - pending_time >= DEBOUNCE_MS) {
    if (gpio_get_level(BTN_GPIO) == 0) {
      press_count++;
      ESP_LOGI(TAG, "press count=%lu", (unsigned long)press_count);
    }
    pending_time = 0;
  }
  return;
}
void app_main(void) {
  setup();
  uint32_t now;

  while (1) {
    now = millis();
    handle_button(now);
    request_toggle(now);
    vTaskDelay(1);
  }
}

#endif /* MODE */
