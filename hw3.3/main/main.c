#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc.h"
#include "led.h"
#include "motor.h"
#include "pwm.h"

static const char *TAG = "adc";

void app_main(void)
{
    setup_adc();
    // setup_led();
    setup_pwm();

    int log_count = 0;

    while (1) {
        int raw = adc_read();
        int pwm_val = pot_to_duty(raw);
        pwm_set_duty(pwm_val);

        if (++log_count >= 20) {
            ESP_LOGI(TAG, "GPIO7 raw: %d", raw);
            log_count = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
