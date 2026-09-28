#ifndef MOTOR_H
#define MOTOR_H

#include <stdint.h>

// maps a raw ADC reading from the pot to a PWM duty (0 = off)
uint8_t pot_to_motor_duty(int adc);

#endif
