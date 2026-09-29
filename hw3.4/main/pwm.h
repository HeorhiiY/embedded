#ifndef PWM_H
#define PWM_H

#include <stdint.h>

#define PWM_MAX_DUTY ((1 << 8) - 1)                 // 8-bit resolution -> 0..255
#define PWM_DUTY_HALF ((PWM_MAX_DUTY + 1) / 2)      // square wave: best volume on a buzzer

// one LEDC channel driving one pin, frequency is changed on the fly to play tones
void setup_pwm(int gpio_num);
void pwm_set_freq(uint32_t hz);
void pwm_set_duty(uint32_t duty);
void pwm_off(void);

#endif
