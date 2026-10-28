#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_adc/adc_oneshot.h"
#include "driver/gpio.h"
#include "adc.h"

#define ADC_GPIO     GPIO_NUM_7
#define ADC_ATTEN    ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12
#define SMA_N 16

static adc_oneshot_unit_handle_t s_adc;
static adc_channel_t s_channel;

static const char *TAG = "adc";

void setup_adc(void)
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

int adc_read(void)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc, s_channel, &raw));
    return raw;
}
