#pragma once

// the lock's status indicator: three LEDs on fixed pins, exactly one lit at a time
void setup_lock_leds(void);
void switch_to_green(void);
void switch_to_yellow(void);
void switch_to_red(void);
