#include "driver/gpio.h"
#include "led.h"

#define LED_GPIO GPIO_NUM_9
#define LEVEL_DARK 2800
#define LEVEL_LIGHT 2200

void setup_led(void)
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

void switch_led(int avg)
{
    if (avg >= LEVEL_DARK) {
        gpio_set_level(LED_GPIO, 1);
    }
    else if (avg <= LEVEL_LIGHT){
        gpio_set_level(LED_GPIO, 0);
    }
    return;

}
