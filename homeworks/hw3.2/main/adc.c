#include "esp_adc/adc_oneshot.h"
#include "esp_adc/adc_cali.h"
#include "esp_adc/adc_cali_scheme.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "adc.h"

#define ADC_GPIO     GPIO_NUM_7
#define ADC_ATTEN    ADC_ATTEN_DB_12
#define ADC_BITWIDTH ADC_BITWIDTH_12

static adc_oneshot_unit_handle_t s_adc;
static adc_unit_t s_unit;
static adc_channel_t s_channel;
static adc_cali_handle_t s_cali;

static const char *TAG = "adc";

// 1) the converter itself: pin -> unit/channel, driver instance, channel config
static void setup_adc_oneshot(void)
{
    // GPIO7 maps to unit ADC1, channel 6
    // ADC1 has: ADC1_CH0 = GPIO1 ... ADC1_CH9 = GPIO10
    ESP_ERROR_CHECK(adc_oneshot_io_to_channel(ADC_GPIO, &s_unit, &s_channel));

    adc_oneshot_unit_init_cfg_t init = {
        .unit_id = s_unit,
    };
    ESP_ERROR_CHECK(adc_oneshot_new_unit(&init, &s_adc));

    adc_oneshot_chan_cfg_t ch = {
        .bitwidth = ADC_BITWIDTH,
        .atten = ADC_ATTEN,
    };
    ESP_ERROR_CHECK(adc_oneshot_config_channel(s_adc, s_channel, &ch));

    ESP_LOGI(TAG, "GPIO%d -> ADC%d_CH%d", (int)ADC_GPIO, (int)s_unit + 1, (int)s_channel);
}

// 2) raw -> millivolts conversion, using per-chip factory values from eFuse
static void setup_adc_calibration(void)
{
    adc_cali_curve_fitting_config_t cali_cfg = {
        .unit_id  = s_unit,
        .chan     = s_channel,
        .atten    = ADC_ATTEN,      // must match the channel config
        .bitwidth = ADC_BITWIDTH,
    };
    ESP_ERROR_CHECK(adc_cali_create_scheme_curve_fitting(&cali_cfg, &s_cali));

    ESP_LOGI(TAG, "calibration: curve fitting");
}

void setup_adc(void)
{
    setup_adc_oneshot();        // must run first: calibration needs unit/channel
    setup_adc_calibration();
}

// raw code 0..4095, uncalibrated (like analogRead)
int adc_read(void)
{
    int raw;
    ESP_ERROR_CHECK(adc_oneshot_read(s_adc, s_channel, &raw));
    return raw;
}

// read + convert in one call (like analogReadMilliVolts)
int adc_read_mv(void)
{
    int mv;
    ESP_ERROR_CHECK(adc_oneshot_get_calibrated_result(s_adc, s_cali, s_channel, &mv));
    return mv;
}

// // convert an already-read raw value, e.g. an averaged one
// int adc_raw_to_mv(int raw)
// {
//     int mv;
//     ESP_ERROR_CHECK(adc_cali_raw_to_voltage(s_cali, raw, &mv));
//     return mv;
// }