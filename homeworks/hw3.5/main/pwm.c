#include "pwm.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define PWM_HZ 50
#define PERIOD_US 20000u
#define PWM_RES LEDC_TIMER_13_BIT
#define PWM_PERIOD_TICKS (1u << PWM_RES)  // 8192 duty steps -> ~2.4 us per step
#define PWM_TIMER LEDC_TIMER_0
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_MODE LEDC_LOW_SPEED_MODE

static const char *TAG = "pwm";

// timer = the clock source: period (50 Hz) and how many duty steps fit in it.
// channel = the output: binds a GPIO to that timer. The driver does what the
// register version did by hand: divider math, clock gating, GPIO matrix routing.
void setup_pwm(int gpio) {
  ledc_timer_config_t timer = {
      .speed_mode = PWM_MODE,
      .duty_resolution = PWM_RES,
      .timer_num = PWM_TIMER,
      .freq_hz = PWM_HZ,
      .clk_cfg = LEDC_AUTO_CLK,  // let the driver pick APB/XTAL
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer));

  ledc_channel_config_t ch = {
      .gpio_num = gpio,
      .speed_mode = PWM_MODE,
      .channel = PWM_CHANNEL,
      .timer_sel = PWM_TIMER,
      .duty = 0,                                 // start with the output low
      .hpoint = 0,                               // pulse starts at period begin
      .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,  // keep the servo held in light-sleep
      .flags.output_invert = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ch));

  ESP_LOGI(TAG, "GPIO%d -> channel %d, %d Hz", gpio, (int)PWM_CHANNEL, PWM_HZ);
}

// pulse width -> duty steps: 1000 us ~ 410, 1500 us ~ 614, 2000 us ~ 819
void pwm_set_pulse_us(uint32_t pulse_us) {
  if (pulse_us > PERIOD_US) {
    pulse_us = PERIOD_US;
  }

  const uint32_t duty = (pulse_us * PWM_PERIOD_TICKS) / PERIOD_US;
  ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, PWM_CHANNEL));  // latch it
}
