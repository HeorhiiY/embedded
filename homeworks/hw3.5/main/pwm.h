#pragma once
#include <stdint.h>

// one LEDC channel driving a servo: fixed 50 Hz, pulse width is what moves the horn
void setup_pwm(int gpio);
void pwm_set_pulse_us(uint32_t pulse_us);
