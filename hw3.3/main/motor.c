#include <stdbool.h>
#include "motor.h"

#define ADC_MAX   4095
#define DUTY_MIN  110   // lowest duty where the motor reliably starts: measure it!
#define DUTY_MAX  255
#define POT_OFF   200   // ADC counts: knob below this = motor off
#define HYST      50    // hysteresis band around POT_OFF

uint8_t pot_to_motor_duty(int adc)
{
    static bool running = false;

    // hysteresis: turn on above POT_OFF+HYST, off below POT_OFF-HYST
    if (running && adc < POT_OFF - HYST)       running = false;
    else if (!running && adc > POT_OFF + HYST) running = true;

    if (!running) return 0;

    if (adc < POT_OFF) adc = POT_OFF;   // clamp inside the hysteresis band

    // linear map [POT_OFF .. ADC_MAX] -> [DUTY_MIN .. DUTY_MAX]
    return DUTY_MIN + (adc - POT_OFF) * (DUTY_MAX - DUTY_MIN) / (ADC_MAX - POT_OFF);
}
