#include "lock_leds.h"
#include "driver/gpio.h"
#include "esp_log.h"
#include "led.h"

// change these to match the wiring
#define LED_GREEN GPIO_NUM_10
#define LED_YELLOW GPIO_NUM_11
#define LED_RED GPIO_NUM_12

static const char *TAG = "lock_leds";

// every switch writes all three pins, so the indicator can't end up showing two
// colours at once no matter what order the callers fire in
static void show_only(int gpio) {
  led_switch(LED_GREEN, gpio == LED_GREEN);
  led_switch(LED_YELLOW, gpio == LED_YELLOW);
  led_switch(LED_RED, gpio == LED_RED);
}

void setup_lock_leds(void) {
  setup_led(LED_GREEN);
  setup_led(LED_YELLOW);
  setup_led(LED_RED);

  ESP_LOGI(TAG, "green=GPIO%d yellow=GPIO%d red=GPIO%d", (int)LED_GREEN, (int)LED_YELLOW,
           (int)LED_RED);
}

void switch_to_green(void) {
  show_only(LED_GREEN);
}

void switch_to_yellow(void) {
  show_only(LED_YELLOW);
}

void switch_to_red(void) {
  show_only(LED_RED);
}
