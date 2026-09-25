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



static volatile uint32_t edges_count = 0;
static uint32_t last_count = 0;
static uint32_t last_press_time = 0;
static uint32_t now = 0;
static uint32_t press_count = 0;

static void IRAM_ATTR gpio_isr_handler(void* arg) {
  (void)arg;
  edges_count++;
}

static inline uint32_t millis(void) {
    return (uint32_t)(esp_timer_get_time() / 1000);
}

// the version without gpio_config, direct usage of the functions
static void setup(void){
  gpio_reset_pin(BTN_GPIO);
  gpio_set_direction(BTN_GPIO, GPIO_MODE_INPUT);
  gpio_set_pull_mode(BTN_GPIO, GPIO_PULLUP_ONLY);
  gpio_set_intr_type(BTN_GPIO, GPIO_INTR_NEGEDGE);

  gpio_install_isr_service(ESP_INTR_FLAG_IRAM);
  gpio_isr_handler_add(BTN_GPIO, gpio_isr_handler, NULL);
  gpio_intr_enable(BTN_GPIO);
}

void app_main(void) {
  setup();
  // ESP_LOGI(TAG, "ready: BTN%d", (int)BTN_GPIO);
#if MODE == 1
  while (1) {
    if (edges_count != last_count) {
      ESP_LOGI(TAG, "press count=%lu", (unsigned long)edges_count);
      last_count = edges_count;
    }
    vTaskDelay(pdMS_TO_TICKS(1000));
  }
#elif MODE == 2
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




#else

#define LED_GPIO GPIO_NUM_41
#define BTN_GPIO GPIO_NUM_21
#define DEBOUNCE_MS 40
#define RELEASE_GUARD_MS 40

static volatile bool btn_irq_pending = false; /* прапорець з ISR */
static bool led_on = false;
static uint32_t press_count = 0;

/* ISR: лише сигнал, без debounce і без delay */
static void IRAM_ATTR gpio_isr_handler(void* arg) {
  (void)arg;
  btn_irq_pending = true;
}

static void setup_led(void) {
  gpio_reset_pin(LED_GPIO);
  gpio_set_direction(LED_GPIO, GPIO_MODE_OUTPUT);
  gpio_set_level(LED_GPIO, 0);
}

static void setup_button_irq(void) {
  gpio_config_t io = {
      .pin_bit_mask = 1ULL << BTN_GPIO, /* маска: біт N = GPIO N */
      .mode = GPIO_MODE_INPUT,
      .pull_up_en = GPIO_PULLUP_ENABLE, /* софтверний pull-up */
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_NEGEDGE, /* фронт натиску (на GND) */
  };
  ESP_ERROR_CHECK(gpio_config(&io));
  ESP_ERROR_CHECK(gpio_install_isr_service(ESP_INTR_FLAG_IRAM));
  ESP_ERROR_CHECK(gpio_isr_handler_add(BTN_GPIO, gpio_isr_handler, NULL));
}

/* Debounce у задачі: підтвердити натиск, toggle, дочекатися відпускання */
static void handle_button_if_needed(void) {
  if (!btn_irq_pending) {
    return;
  }

  btn_irq_pending = false;
  vTaskDelay(pdMS_TO_TICKS(DEBOUNCE_MS));
  if (gpio_get_level(BTN_GPIO) != 0) {
    return; /* шум / вже відпустили */
  }

  led_on = !led_on;
  gpio_set_level(LED_GPIO, led_on ? 1 : 0);
  press_count++;
  ESP_LOGI(TAG, "press #%lu led=%s", (unsigned long)press_count, led_on ? "ON" : "OFF");

  while (gpio_get_level(BTN_GPIO) == 0) {
    vTaskDelay(pdMS_TO_TICKS(10));
  }
  vTaskDelay(pdMS_TO_TICKS(RELEASE_GUARD_MS));
  btn_irq_pending = false; /* скинути зайві IRQ за час жесту */
}

void app_main(void) {
  setup_led();
  setup_button_irq();
  ESP_LOGI(TAG, "ready: BTN%d -> LED%d", (int)BTN_GPIO, (int)LED_GPIO);

  while (1) {
    handle_button_if_needed();
    vTaskDelay(pdMS_TO_TICKS(10));
  }
}
#endif
}