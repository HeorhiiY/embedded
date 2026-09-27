#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc.h"


static const char *TAG = "adc";


int compute_approximate_voltage(int raw){
    return 3100*raw/4095; // mV
}

void app_main(void)
{
    setup_adc();

    ESP_LOGI(TAG, "RAW   U_manual(mV)   U_cali(mV)   Error(%%)");
    ESP_LOGI(TAG, "------------------------------------------");
    ESP_LOGI(TAG, "");

    while (1) {
        int raw = adc_read();
        int voltage = adc_read_mv();
        int approximate_voltage = compute_approximate_voltage(raw);
        float error = ((float)(approximate_voltage - voltage) / voltage) * 100;

        ESP_LOGI(TAG, "%5d    %10d    %10d    %8.2f", raw, approximate_voltage, voltage, error);

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}
