#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "esp_log.h"

#define ADC_GPIO     GPIO_NUM_7
#define ADC_ATTEN    ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define LED_GPIO GPIO_NUM_9
#define SMA_N 16

// setup for the evening, close to the sunset
#define LEVEL_DARK 2800
#define LEVEL_LIGHT 2200


static adc_oneshot_unit_handle_t s_adc;
static adc_channel_t s_channel;

static const char *TAG = "adc";

static void setup_adc(void)
{
    adc_unit_t unit = 0;
    // for GPIO_7 maps to unit-> ADC1 and Channel -> channel 6
    // the ADC1 has: ADC1_CH0	GPIO1 ... to ADC1_CH9 = GPIO10
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(ADC_GPIO, &unit, &s_channel));

    adc_oneshot_unit_init_cfg_t init = {
        .unit_id = unit,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init, &s_adc));

    adc_oneshot_chan_cfg_t ch = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, s_channel, &ch));
}

static void setup_led(void)
{
    gpio_config_t cfg = {
        .pin_bit_mask = 1ULL << LED_GPIO,        // which pin(s): bit 9 set
        .mode         = GPIO_MODE_OUTPUT,
        .pull_up_en   = GPIO_PULLUP_DISABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,
    };
    ESP_ERROR_CHECK(gpio_config(&cfg));
    gpio_set_level(LED_GPIO, 0);               // start LOW
}


static void switch_led(int avg)
{
    if (avg >= LEVEL_DARK) {
        gpio_set_level(LED_GPIO, 1);
    }
    else if (avg <= LEVEL_LIGHT){
        gpio_set_level(LED_GPIO, 0);
    }
    return;
    
}
// 


// averaging

typedef struct {
    int     buf[SMA_N];   /* last N samples */
    int     idx;          /* where the next sample goes */
    int     count;        /* samples collected so far (up to N) */
    int32_t sum;          /* running sum of buf */
} sma_t;

static sma_t setup_averaging(void)
{
    return (sma_t){0};
}

// funny version that allows to not read the whole buffer every time, 
// by moving the index which will point to the oldest value
static int sma_update(sma_t *s, int x)
{
    s->sum += x - s->buf[s->idx];   /* add newest, remove oldest */
    s->buf[s->idx] = x;
    s->idx = (s->idx + 1) % SMA_N;
    if (s->count < SMA_N) s->count++;
    return s->sum / s->count;       /* correct during warm-up too */
}

static void warm_up(sma_t *s){
    // fill in 5 vals in warm up stage
    for (int i = 0; i < 5; i++){
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc, s_channel, &raw));
        sma_update(s, raw);
        vTaskDelay(pdMS_TO_TICKS(20));
    }
}


void app_main(void)
{
    setup_adc();
    setup_led();
    sma_t avg = setup_averaging();
    int avg_val = 0;
    warm_up(&avg);

    int log_count = 0;

    while (1) {
        int raw;
        ESP_ERROR_CHECK(adc_oneshot_read(s_adc, s_channel, &raw));
        avg_val = sma_update(&avg, raw);
        switch_led(avg_val);

        if (++log_count >= 50) {
            ESP_LOGI(TAG, "GPIO7 raw: %d, average: %d", raw, avg_val);
            log_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
