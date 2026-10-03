#include "led.h"
#include "driver/gpio.h"
#include "esp_log.h"

static const char *TAG = "led";

// gpio_config does in one call what the register version did by hand: output
// enable, pull resistors off, interrupts off, and routing through the GPIO matrix
void setup_led(int gpio) {
  gpio_config_t cfg = {
      .pin_bit_mask = 1ULL << gpio,  // 64-bit mask: one bit per pin
      .mode = GPIO_MODE_OUTPUT,
      .pull_up_en = GPIO_PULLUP_DISABLE,
      .pull_down_en = GPIO_PULLDOWN_DISABLE,
      .intr_type = GPIO_INTR_DISABLE,
  };
  ESP_ERROR_CHECK(gpio_config(&cfg));
  ESP_ERROR_CHECK(gpio_set_level(gpio, 0));  // start off

  ESP_LOGI(TAG, "GPIO%d -> output, off", gpio);
}

// the digitalWrite equivalent: one register write, no delay, returns immediately
void led_switch(int gpio, bool on) {
  ESP_ERROR_CHECK(gpio_set_level(gpio, on ? 1 : 0));
}
