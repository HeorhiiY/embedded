#include <stdbool.h>
#include <stdint.h>
#include "driver/gpio.h"
#include "esp_err.h"
#include "esp_log.h"
#include "button.h"

#define BTN_GPIO          GPIO_NUM_5      /* pull-up, button to GND: 0 = pressed */
#define DEBOUNCE_SAMPLES  6                /* 6 x 10 ms = 60 ms stable level */
#define LONG_PRESS_MS     800
#define LONG_PRESS_POLLS  (LONG_PRESS_MS / BUTTON_POLL_PERIOD_MS)

static const char *TAG = "button";

typedef enum {
    BTN_OFF,
    BTN_PRESSED,
    BTN_LONG_PRESS,
} btn_state_t;

static btn_state_t state = BTN_OFF;
static bool     stable_pressed = false;   /* debounced level */
static uint8_t  change_count   = 0;       /* consecutive samples differing from stable level */
static uint32_t held_polls     = 0;       /* polls spent in BTN_PRESSED */

/* Counter debounce: accept a new level only after DEBOUNCE_SAMPLES
 * identical samples in a row; any sample matching the current stable
 * level resets the counter. */
static bool debounce(bool raw_pressed)
{
    if (raw_pressed == stable_pressed) {
        change_count = 0;
    } else if (++change_count >= DEBOUNCE_SAMPLES) {
        stable_pressed = raw_pressed;
        change_count = 0;
    }
    return stable_pressed;
}

void button_init(void)
{
    const gpio_config_t io = {
        .pin_bit_mask = 1ULL << BTN_GPIO,
        .mode         = GPIO_MODE_INPUT,
        .pull_up_en   = GPIO_PULLUP_ENABLE,
        .pull_down_en = GPIO_PULLDOWN_DISABLE,
        .intr_type    = GPIO_INTR_DISABLE,     /* polled, no interrupt */
    };
    ESP_ERROR_CHECK(gpio_config(&io));
}

button_event_t button_poll(void)
{
    const bool raw     = (gpio_get_level(BTN_GPIO) == 0);
    const bool pressed = debounce(raw);
    button_event_t ev  = BUTTON_EVENT_NONE;

    switch (state) {
    case BTN_OFF:
        if (pressed) {
            state = BTN_PRESSED;
            held_polls = 0;
        }
        break;

    case BTN_PRESSED:
        if (!pressed) {
            state = BTN_OFF;
            ev = BUTTON_EVENT_SHORT;
            ESP_LOGI(TAG, "short press");
        } else if (++held_polls >= LONG_PRESS_POLLS) {
            state = BTN_LONG_PRESS;
            ev = BUTTON_EVENT_LONG;
            ESP_LOGI(TAG, "long press");
        }
        break;

    case BTN_LONG_PRESS:
        if (!pressed) {
            state = BTN_OFF;               /* release after long: no extra event */
        }
        break;
    }

    return ev;
}