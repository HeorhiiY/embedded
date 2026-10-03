#include "pcnt.h"
#include "driver/gpio.h"
#include "driver/pulse_cnt.h"
#include "esp_log.h"
#include <stdatomic.h>


#define GPIO_A GPIO_NUM_6
#define GPIO_B GPIO_NUM_5
#define GLITCH_NS 10000

static const char *TAG = "pcnt";

static pcnt_unit_handle_t s_pcnt;

static _Atomic int32_t s_position = 0;


static bool on_reach_cb(pcnt_unit_handle_t unit,
                        const pcnt_watch_event_data_t *edata, void *user_ctx)
{
    int32_t step = (edata->watch_point_value > 0) ? +1 : -1;
    atomic_fetch_add_explicit(&s_position, step, memory_order_relaxed);
    return false;
}

void pcnt_encoder_init(void) {
  pcnt_unit_config_t unit_cfg = {
      .high_limit = +4,
      .low_limit = -4,
  };
  ESP_ERROR_CHECK(pcnt_new_unit(&unit_cfg, &s_pcnt));

  // fix from Heorhii: set glitch filter

  pcnt_glitch_filter_config_t filter_cfg = {
      .max_glitch_ns = GLITCH_NS,
  };
  ESP_ERROR_CHECK(pcnt_unit_set_glitch_filter(s_pcnt, &filter_cfg));

  // add watchpoints at +-4
  pcnt_unit_add_watch_point(s_pcnt,  4);
  pcnt_unit_add_watch_point(s_pcnt, -4);


  /// end fix from Heorhii

  pcnt_chan_config_t ch_a_cfg = {
      .edge_gpio_num = GPIO_A,
      .level_gpio_num = GPIO_B,
  };
  pcnt_channel_handle_t ch_a;
  ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt, &ch_a_cfg, &ch_a));
  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(ch_a, PCNT_CHANNEL_EDGE_ACTION_DECREASE,
                                               PCNT_CHANNEL_EDGE_ACTION_INCREASE));
  ESP_ERROR_CHECK(pcnt_channel_set_level_action(ch_a, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

  pcnt_chan_config_t ch_b_cfg = {
      .edge_gpio_num = GPIO_B,
      .level_gpio_num = GPIO_A,
  };
  pcnt_channel_handle_t ch_b;
  ESP_ERROR_CHECK(pcnt_new_channel(s_pcnt, &ch_b_cfg, &ch_b));
  ESP_ERROR_CHECK(pcnt_channel_set_edge_action(ch_b, PCNT_CHANNEL_EDGE_ACTION_INCREASE,
                                               PCNT_CHANNEL_EDGE_ACTION_DECREASE));
  ESP_ERROR_CHECK(pcnt_channel_set_level_action(ch_b, PCNT_CHANNEL_LEVEL_ACTION_KEEP,
                                                PCNT_CHANNEL_LEVEL_ACTION_INVERSE));

  ESP_ERROR_CHECK(pcnt_unit_enable(s_pcnt));
  ESP_ERROR_CHECK(pcnt_unit_clear_count(s_pcnt));
  ESP_ERROR_CHECK(pcnt_unit_start(s_pcnt));

  ESP_LOGI(TAG, "A=GPIO%d B=GPIO%d, x4 decode, glitch filter %d ns", (int)GPIO_A, (int)GPIO_B,
           GLITCH_NS);
}

int pcnt_encoder_get_count(void) {
  int count = 0;
  ESP_ERROR_CHECK(pcnt_unit_get_count(s_pcnt, &count));
  return count;
}
