#include <stdio.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "esp_log.h"
#include "adc.h"
#include "led.h"
#include "motor.h"
#include "pwm.h"

static const char *TAG = "adc";

#define MOTOR_GPIO 8
#define LED_GPIO 9

void app_main(void)
{
    setup_adc();
    // setup_led();
    pwm_handle_t motor = setup_pwm(MOTOR_GPIO);
    pwm_handle_t led = setup_pwm(LED_GPIO);

    int log_count = 0;

    while (1) {
        int raw = adc_read();
        int pwm_val_motor = pot_to_motor_duty(raw);
        int pwm_val_led = pot_to_led_duty(raw);
        pwm_set_duty(motor, pwm_val_motor);
        pwm_set_duty(led, pwm_val_led);

        if (++log_count >= 20) {
            ESP_LOGI(TAG, "GPIO7 raw: %d", raw);
            log_count = 0;
        }
        
        vTaskDelay(pdMS_TO_TICKS(50));
    }
}
