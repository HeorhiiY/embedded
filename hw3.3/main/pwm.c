#include "driver/gpio.h"
#include "driver/ledc.h"
#include "pwm.h"

#define PWM_GPIO    GPIO_NUM_8
#define PWM_FREQ_HZ 1000
#define PWM_TIMER   LEDC_TIMER_0
#define PWM_CHANNEL LEDC_CHANNEL_0
#define PWM_MODE    LEDC_LOW_SPEED_MODE
#define PWM_RES     LEDC_TIMER_8_BIT

void setup_pwm(void)
{
    // timer = the clock source: how fast one PWM period is (freq) and
    // how many duty steps fit inside it (resolution)
    ledc_timer_config_t timer = {
        .speed_mode      = PWM_MODE,
        .duty_resolution = PWM_RES,        // 8 bit -> duty 0..255
        .timer_num       = PWM_TIMER,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,  // let the driver pick APB/XTAL
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));

    // channel = the output: binds one GPIO to that timer
    ledc_channel_config_t ch = {
        .gpio_num    = PWM_GPIO,
        .speed_mode  = PWM_MODE,
        .channel     = PWM_CHANNEL,
        .timer_sel   = PWM_TIMER,
        .duty        = 0,                          // start off
        .hpoint      = 0,                          // pulse starts at period begin
        .sleep_mode  = LEDC_SLEEP_MODE_KEEP_ALIVE, // keep output in light-sleep
        .flags.output_invert = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));
}

void pwm_set_duty(uint32_t duty)
{
    if (duty > PWM_MAX_DUTY) {
        duty = PWM_MAX_DUTY;
    }
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, PWM_CHANNEL, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, PWM_CHANNEL));  // latch it
}
