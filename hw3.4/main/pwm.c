#include "pwm.h"
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"

#define PWM_FREQ_HZ 2000  // placeholder, every note overwrites it
#define PWM_TIMER LEDC_TIMER_0
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_MODE LEDC_LOW_SPEED_MODE
#define PWM_RES LEDC_TIMER_8_BIT

static const char* TAG = "pwm";

// timer = the clock source: how fast one PWM period is (freq) and how many duty
// steps fit inside it (resolution). channel = the output: binds a GPIO to it.
void setup_pwm(int gpio_num) {
  ledc_timer_config_t timer = {
      .speed_mode = PWM_MODE,
      .duty_resolution = PWM_RES,
      .timer_num = PWM_TIMER,
      .freq_hz = PWM_FREQ_HZ,
      .clk_cfg = LEDC_AUTO_CLK,  // let the driver pick APB/XTAL
  };
  ESP_ERROR_CHECK(ledc_timer_config(&timer));
  ledc_channel_config_t ch = {
      .gpio_num = gpio_num,
      .speed_mode = PWM_MODE,
      .channel = PWM_CHANNEL,
      .timer_sel = PWM_TIMER,
      .duty = 0,                                 // start off
      .hpoint = 0,                               // pulse starts at period begin
      .sleep_mode = LEDC_SLEEP_MODE_KEEP_ALIVE,  // keep output in light-sleep
      .flags.output_invert = 0,
  };
  ESP_ERROR_CHECK(ledc_channel_config(&ch));
  ESP_LOGI(TAG, "GPIO%d -> channel %d", gpio_num, (int)PWM_CHANNEL);
}

// retunes the shared timer, so the pitch of the channel follows
void pwm_set_freq(uint32_t hz) {
  ESP_ERROR_CHECK(ledc_set_freq(PWM_MODE, PWM_TIMER, hz));
}

void pwm_set_duty(uint32_t duty) {
  if (duty > PWM_MAX_DUTY) {
    duty = PWM_MAX_DUTY;
  }
  ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty));
  ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, PWM_CHANNEL));  // latch it
}

void pwm_off(void) {
  pwm_set_duty(0);
}
