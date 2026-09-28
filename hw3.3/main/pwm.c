#include <stdbool.h>
#include "driver/gpio.h"
#include "driver/ledc.h"
#include "esp_log.h"
#include "pwm.h"

#define PWM_FREQ_HZ 1000
#define PWM_TIMER   LEDC_TIMER_0
#define PWM_MODE    LEDC_LOW_SPEED_MODE
#define PWM_RES     LEDC_TIMER_8_BIT
#define PWM_MAX_CH  8            // low-speed LEDC channels on the ESP32-S3

struct pwm_s {
    ledc_channel_t channel;
    int            gpio;
};

// no malloc: handles are handed out of a fixed pool, one per channel
static struct pwm_s s_pool[PWM_MAX_CH];
static int  s_used = 0;
static bool s_timer_ready = false;

static const char *TAG = "pwm";

// timer = the clock source: how fast one PWM period is (freq) and how many
// duty steps fit inside it (resolution). All channels share it, so it is
// configured once, on the first setup_pwm() call.
static void setup_timer(void)
{
    ledc_timer_config_t timer = {
        .speed_mode      = PWM_MODE,
        .duty_resolution = PWM_RES,        // 8 bit -> duty 0..255
        .timer_num       = PWM_TIMER,
        .freq_hz         = PWM_FREQ_HZ,
        .clk_cfg         = LEDC_AUTO_CLK,  // let the driver pick APB/XTAL
    };
    ESP_ERROR_CHECK(ledc_timer_config(&timer));
}

pwm_handle_t setup_pwm(int gpio_num)
{
    if (s_used >= PWM_MAX_CH) {
        ESP_LOGE(TAG, "no free channel left for GPIO%d", gpio_num);
        return NULL;
    }

    if (!s_timer_ready) {
        setup_timer();
        s_timer_ready = true;
    }

    struct pwm_s *pwm = &s_pool[s_used];
    pwm->channel = (ledc_channel_t)s_used;
    pwm->gpio    = gpio_num;
    s_used++;

    // channel = the output: binds one GPIO to that timer
    ledc_channel_config_t ch = {
        .gpio_num    = gpio_num,
        .speed_mode  = PWM_MODE,
        .channel     = pwm->channel,
        .timer_sel   = PWM_TIMER,
        .duty        = 0,                          // start off
        .hpoint      = 0,                          // pulse starts at period begin
        .sleep_mode  = LEDC_SLEEP_MODE_KEEP_ALIVE, // keep output in light-sleep
        .flags.output_invert = 0,
    };
    ESP_ERROR_CHECK(ledc_channel_config(&ch));

    ESP_LOGI(TAG, "GPIO%d -> channel %d, %d Hz", gpio_num, (int)pwm->channel, PWM_FREQ_HZ);
    return pwm;
}

void pwm_set_duty(pwm_handle_t pwm, uint32_t duty)
{
    if (pwm == NULL) {
        return;
    }
    if (duty > PWM_MAX_DUTY) {
        duty = PWM_MAX_DUTY;
    }
    ESP_ERROR_CHECK(ledc_set_duty(PWM_MODE, pwm->channel, duty));
    ESP_ERROR_CHECK(ledc_update_duty(PWM_MODE, pwm->channel));  // latch it
}
