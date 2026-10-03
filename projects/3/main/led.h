#pragma once
#include <stdbool.h>

// plain push-pull outputs. no state kept here: configure each pin once, then
// address it by number, like digitalWrite. call setup_led per pin.
void setup_led(int gpio);
void led_switch(int gpio, bool on);
