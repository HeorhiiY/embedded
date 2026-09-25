#include <stdio.h>
#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/gptimer.h"
#include "esp_log.h"
#include "esp_timer.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"

static const char* TAG = "2.5-tmr";

#define TIMER_RESOLUTION_HZ 1000000u /* 1 tick = 1 us  */
#define ALARM_PERIOD_US 3000000u      /* switch every 3s */
#define POLL_PERIOD_MS 1000           /* log interval */
#define CONTROL_PIN GPIO_NUM_7

volatile bool motor_on = false;
static gptimer_handle_t gptimer_alarm;

static volatile uint32_t alarm_count = 0;


static bool IRAM_ATTR on_timer_alarm(gptimer_handle_t timer,
                                     const gptimer_alarm_event_data_t* edata, void* user_ctx) {
  (void)timer;
  (void)edata;
  (void)user_ctx;
  motor_on = !motor_on;
  gpio_set_level(CONTROL_PIN, motor_on ? 1 : 0);
  alarm_count++;
  return false;
}

/* Канал A: періодичний alarm + auto-reload */
static void setup_gptimer_alarm(void) {
  gptimer_config_t config = {
      .clk_src = GPTIMER_CLK_SRC_DEFAULT,
      .direction = GPTIMER_COUNT_UP,
      .resolution_hz = TIMER_RESOLUTION_HZ,
  };
  ESP_ERROR_CHECK(gptimer_new_timer(&config, &gptimer_alarm));

  gptimer_event_callbacks_t cbs = {
      .on_alarm = on_timer_alarm,
  };
  ESP_ERROR_CHECK(gptimer_register_event_callbacks(gptimer_alarm, &cbs, NULL));

  gptimer_alarm_config_t alarm = {
      .reload_count = 0,
      .alarm_count = ALARM_PERIOD_US,
      .flags.auto_reload_on_alarm = true,
  };
  ESP_ERROR_CHECK(gptimer_set_alarm_action(gptimer_alarm, &alarm));
  ESP_ERROR_CHECK(gptimer_enable(gptimer_alarm));
  ESP_ERROR_CHECK(gptimer_start(gptimer_alarm));
}

static void setup_control() {
    gpio_set_level(CONTROL_PIN, 0);  // preset output latch before enabling the driver
    gpio_config_t io = {
        .pin_bit_mask = 1ULL << CONTROL_PIN,
        .mode = GPIO_MODE_OUTPUT,
        .pull_up_en = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&io));
}

void app_main(void) {
  setup_gptimer_alarm();
  setup_control();

  ESP_LOGI(TAG, "Start the test");

  while (1) {
    ESP_LOGI(TAG, "alarms=%lu motor=%s", (unsigned long)alarm_count,
             motor_on ? "ON" : "OFF");
    vTaskDelay(pdMS_TO_TICKS(POLL_PERIOD_MS));
  }
}
