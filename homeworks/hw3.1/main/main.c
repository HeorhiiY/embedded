#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc.h"
#include "led.h"

static const char *TAG = "adc";


void app_main(void)
{
    setup_adc();
    setup_led();
    sma_t avg = setup_averaging();
    int avg_val = 0;
    warm_up(&avg);

    int log_count = 0;

    while (1) {
        int raw = adc_read();
        avg_val = sma_update(&avg, raw);
        switch_led(avg_val);

        if (++log_count >= 50) {
            ESP_LOGI(TAG, "GPIO7 raw: %d, average: %d", raw, avg_val);
            log_count = 0;
        }

        vTaskDelay(pdMS_TO_TICKS(20));
    }
}
