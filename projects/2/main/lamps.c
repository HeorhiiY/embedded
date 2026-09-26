#include "lamps.h"
#include "driver/gpio.h"
#include "esp_timer.h"
#include <stdbool.h>

#define GPIO_RED    GPIO_NUM_16
#define GPIO_YELLOW GPIO_NUM_17
#define GPIO_GREEN  GPIO_NUM_18

#define BLINK_PERIOD_US (500 * 1000)

typedef enum { LAMP_R, LAMP_Y, LAMP_G, LAMP_COUNT } lamp_id_t;

static const gpio_num_t lamp_pins[LAMP_COUNT] = { GPIO_RED, GPIO_YELLOW, GPIO_GREEN };
static lamp_mode_t lamp_mode[LAMP_COUNT];
static bool blink_phase = true;
static esp_timer_handle_t blink_timer;

static void lamps_render(void)
{
    for (int i = 0; i < LAMP_COUNT; i++) {
        bool on = (lamp_mode[i] == LAMP_ON) ||
                  (lamp_mode[i] == LAMP_BLINK && blink_phase);
        gpio_set_level(lamp_pins[i], on);
    }
}

static void blink_timer_cb(void *arg)
{
    (void)arg;
    blink_phase = !blink_phase;
    lamps_render();
}

void lamps_init(void)
{
    for (int i = 0; i < LAMP_COUNT; i++) {
        gpio_reset_pin(lamp_pins[i]);
        gpio_set_direction(lamp_pins[i], GPIO_MODE_OUTPUT);
        gpio_set_level(lamp_pins[i], 0);
    }

    const esp_timer_create_args_t args = {
        .callback = blink_timer_cb,
        .name     = "blink",
    };
    ESP_ERROR_CHECK(esp_timer_create(&args, &blink_timer));
    ESP_ERROR_CHECK(esp_timer_start_periodic(blink_timer, BLINK_PERIOD_US));
}

void lamps_set(lamp_mode_t red, lamp_mode_t yellow, lamp_mode_t green)
{
    lamp_mode[LAMP_R] = red;
    lamp_mode[LAMP_Y] = yellow;
    lamp_mode[LAMP_G] = green;
    blink_phase = true;
    esp_timer_stop(blink_timer);
    esp_timer_start_periodic(blink_timer, BLINK_PERIOD_US);
    lamps_render();
}
