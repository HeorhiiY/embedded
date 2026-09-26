#pragma once
 
/* Call button_poll() exactly every BUTTON_POLL_PERIOD_MS from one task. */
#define BUTTON_POLL_PERIOD_MS 10
 
typedef enum {
    BUTTON_EVENT_NONE,
    BUTTON_EVENT_SHORT,   /* released before long-press threshold */
    BUTTON_EVENT_LONG,    /* fired once when threshold is crossed, while still held */
} button_event_t;
 
void button_init(void);
button_event_t button_poll(void);
