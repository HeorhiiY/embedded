#include "traffic.h"
#include "lamps.h"
#include "esp_log.h"

static const char *TAG = "traffic";

typedef enum {
    RED,
    RED_AND_YELLOW,
    GREEN,
    GREEN_BLINKING,
    YELLOW,
    STATE_COUNT
} lights_state_t;

static const char *const state_names[] = {
    [RED]            = "RED",
    [RED_AND_YELLOW] = "RED_AND_YELLOW",
    [GREEN]          = "GREEN",
    [GREEN_BLINKING] = "GREEN_BLINKING",
    [YELLOW]         = "YELLOW",
};

_Static_assert(sizeof(state_names) / sizeof(state_names[0]) == STATE_COUNT,
               "state_names must have STATE_COUNT entries");

static lights_state_t lights_state = RED;

static void set_state(lights_state_t s)
{
    lights_state = s;
    switch (s) {
    case RED:            lamps_set(LAMP_ON,  LAMP_OFF, LAMP_OFF);   break;
    case RED_AND_YELLOW: lamps_set(LAMP_ON,  LAMP_ON,  LAMP_OFF);   break;
    case GREEN:          lamps_set(LAMP_OFF, LAMP_OFF, LAMP_ON);    break;
    case GREEN_BLINKING: lamps_set(LAMP_OFF, LAMP_OFF, LAMP_BLINK); break;
    case YELLOW:         lamps_set(LAMP_OFF, LAMP_ON,  LAMP_OFF);   break;
    default: break;
    }
    ESP_LOGI(TAG, "State: %s", state_names[s]);
}

void traffic_init(void)
{
    set_state(RED);
}

void traffic_next(void)
{
    set_state((lights_state + 1) % STATE_COUNT);
}

void traffic_reset(void)
{
    set_state(RED);
}

