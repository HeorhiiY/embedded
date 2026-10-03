#include <stdbool.h>
#include <stdio.h>
#include "driver/gpio.h"
#include "driver/uart.h"
#include "driver/uart_vfs.h"
#include "esp_log.h"
#include "freertos/FreeRTOS.h"
#include "freertos/projdefs.h"
#include "freertos/task.h"
#include "lock_fsm.h"
#include "lock_leds.h"
#include "pcnt.h"
#include "pwm.h"

static const char *TAG = "hw3.5-servo";

#define SERVO_GPIO GPIO_NUM_8
#define US_AT_0 500
#define US_AT_180 2400
#define HOLD_MS 2000
#define LOG_EVERY 5   // loop ticks between log lines -> one per 250 ms

static const uint8_t SAFE_CODE[LOCK_CODE_LEN] = {1, 2, 3, 4};

static uint32_t deg_to_us(int deg) {
  if (deg < 0) {
    deg = 0;
  }
  if (deg > 180) {
    deg = 180;
  }
  return US_AT_0 + ((uint32_t)deg * (US_AT_180 - US_AT_0)) / 180;
}

// returns the pulse width it applied, so the caller can log it
static uint32_t servo_write_deg(int deg) {
  const uint32_t us = deg_to_us(deg);
  pwm_set_pulse_us(us);
  return us;
}

static void lock_safe(void){
  servo_write_deg(180);
}

static void open_safe(void){
  servo_write_deg(0);
}

// static void hold_deg(int deg) {
//   servo_write_deg(deg);
//   vTaskDelay(pdMS_TO_TICKS(HOLD_MS));
// }

// the only place state -> hardware: BLOCKED holds the bolt shut like LOCKED,
// the LED is what tells the two apart
static void apply_state(lock_state_t state) {
  switch (state) {   /* no default: -Wswitch flags unhandled states */

  case LOCK_STATE_OPEN:
    open_safe();
    switch_to_green();
    break;

  case LOCK_STATE_LOCKED:
    lock_safe();
    switch_to_yellow();
    break;

  case LOCK_STATE_BLOCKED:
    lock_safe();
    switch_to_red();
    break;
  }
}

// without this stdin is non-blocking and getchar() just returns EOF forever:
// the UART driver has to be pushed under the VFS layer first
static void console_init(void) {
  setvbuf(stdin, NULL, _IONBF, 0);  // unbuffered, so a keypress arrives immediately

  const uart_config_t cfg = {
      .baud_rate = CONFIG_ESP_CONSOLE_UART_BAUDRATE,
      .data_bits = UART_DATA_8_BITS,
      .parity = UART_PARITY_DISABLE,
      .stop_bits = UART_STOP_BITS_1,
      .source_clk = UART_SCLK_DEFAULT,
  };
  ESP_ERROR_CHECK(uart_driver_install(CONFIG_ESP_CONSOLE_UART_NUM, 256, 0, 0, NULL, 0));
  ESP_ERROR_CHECK(uart_param_config(CONFIG_ESP_CONSOLE_UART_NUM, &cfg));
  uart_vfs_dev_use_driver(CONFIG_ESP_CONSOLE_UART_NUM);
  uart_vfs_dev_port_set_rx_line_endings(CONFIG_ESP_CONSOLE_UART_NUM, ESP_LINE_ENDINGS_CR);
}

// maps a keypress onto an FSM event; false = key we don't care about
static bool key_to_event(int c, lock_event_t *evt) {
  if (c >= '0' && c <= '9') {
    *evt = (lock_event_t){.type = LOCK_EVT_DIGIT, .digit = (uint8_t)(c - '0')};
    return true;
  }
  if (c == 'l' || c == 'L') {
    *evt = (lock_event_t){.type = LOCK_EVT_LOCK};
    return true;
  }
  if (c == 'u' || c == 'U') {
    *evt = (lock_event_t){.type = LOCK_EVT_UNBLOCK};
    return true;
  }
  if (c == 'r' || c == 'R') {
    *evt = (lock_event_t){.type = LOCK_EVT_RESET};
    return true;
  }
  return false;
}

static const char *evt_name(lock_event_type_t type) {
  switch (type) {   /* no default: -Wswitch flags unhandled events */
  case LOCK_EVT_DIGIT:   return "digit";
  case LOCK_EVT_LOCK:    return "lock";
  case LOCK_EVT_UNBLOCK: return "unblock";
  case LOCK_EVT_RESET:   return "reset";
  }
  return "?";
}

void app_main(void) {
  setup_pwm(SERVO_GPIO);
  setup_lock_leds();
  console_init();

  lock_fsm_t fsm;
  lock_fsm_init(&fsm, SAFE_CODE, LOCK_STATE_LOCKED);
  apply_state(fsm.state);  // LOCKED at boot -> yellow

  ESP_LOGI(TAG, "console lock test, code 1234");
  ESP_LOGI(TAG, "keys: 0-9 = digit, l = lock, u = unblock, r = reset entry, ? = show state");
  ESP_LOGI(TAG, "state=%s", lock_state_name(fsm.state));

  for (;;) {
    const int c = getchar();
    if (c == EOF) {
      vTaskDelay(pdMS_TO_TICKS(20));
      continue;
    }

    if (c == '?') {
      ESP_LOGI(TAG, "state=%s, %u/%d digits, %u failed attempts", lock_state_name(fsm.state),
               (unsigned)fsm.digits_entered, LOCK_CODE_LEN, (unsigned)fsm.failed_attempts);
      continue;
    }

    lock_event_t evt;
    if (!key_to_event(c, &evt)) {
      continue;  // CR/LF and anything else: ignore
    }

    const lock_state_t before = fsm.state;
    const lock_state_t after = lock_fsm_handle_event(&fsm, &evt);

    if (evt.type == LOCK_EVT_DIGIT) {
      ESP_LOGI(TAG, "digit %u -> %u/%d digits, state=%s, %u failed attempts",
               (unsigned)evt.digit, (unsigned)fsm.digits_entered, LOCK_CODE_LEN,
               lock_state_name(after), (unsigned)fsm.failed_attempts);
    } else {
      ESP_LOGI(TAG, "%s -> %u/%d digits, state=%s", evt_name(evt.type),
               (unsigned)fsm.digits_entered, LOCK_CODE_LEN, lock_state_name(after));
    }

    if (after != before) {
      apply_state(after);
      ESP_LOGW(TAG, "%s -> %s, servo to %s", lock_state_name(before), lock_state_name(after),
               after == LOCK_STATE_OPEN ? "0 deg (open)" : "180 deg (locked)");
    }
  }
}
