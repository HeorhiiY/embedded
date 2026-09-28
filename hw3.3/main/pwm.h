#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#define PWM_MAX_DUTY ((1 << 8) - 1)   // 8-bit resolution -> 0..255

// opaque handle: one LEDC channel driving one pin
typedef struct pwm_s *pwm_handle_t;

// claims the next free LEDC channel for that pin, NULL if none left
pwm_handle_t setup_pwm(int gpio_num);
void pwm_set_duty(pwm_handle_t pwm, uint32_t duty);

#endif
