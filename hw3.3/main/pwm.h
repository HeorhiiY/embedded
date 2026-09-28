#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#define PWM_MAX_DUTY ((1 << 8) - 1)   // 8-bit resolution -> 0..255

void setup_pwm(void);
void pwm_set_duty(uint32_t duty);

#endif
