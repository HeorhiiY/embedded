#include <stdio.h>
#include "driver/gpio.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "adc.h"
#include "pwm.h"

static const char *TAG = "hw3.5-servo";

#define SERVO_GPIO GPIO_NUM_5
#define US_AT_0 500
#define US_AT_180 2400
#define HOLD_MS 2000
#define LOG_EVERY 5   // loop ticks between log lines -> one per 250 ms


static uint32_t deg_to_us(int deg) {
  if (deg < 0) {
    deg = 0;
  }
  if (deg > 180) {
    deg = 180;
  }
  return US_AT_0 + ((uint32_t)deg * (US_AT_180 - US_AT_0)) / 180;
}

// returns the pulse width it applied, so the caller can log it
static uint32_t servo_write_deg(int deg) {
  const uint32_t us = deg_to_us(deg);
  pwm_set_pulse_us(us);
  return us;
}

// static void hold_deg(int deg) {
//   servo_write_deg(deg);
//   vTaskDelay(pdMS_TO_TICKS(HOLD_MS));
// }

void app_main(void) {
  setup_pwm(SERVO_GPIO);
  setup_adc();
  ESP_LOGI(TAG, "Servo GPIO%d and ADC initialized", (int)SERVO_GPIO);

  int log_counter = 0;
  for (;;) {
    int raw = adc_read();
    int degree = adc_raw_to_degree(raw);
    uint32_t us = servo_write_deg(degree);

    // the loop runs at 50 ms, one log line per LOG_EVERY ticks keeps it readable
    if (++log_counter >= LOG_EVERY) {
      ESP_LOGI(TAG, "raw %4d -> %3d deg (%u us)", raw, degree, (unsigned)us);
      log_counter = 0;
    }

    vTaskDelay(pdMS_TO_TICKS(50));
  }
}
