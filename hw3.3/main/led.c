#include "driver/gpio.h"
#include "led.h"

uint8_t pot_to_led_duty(int adc)
{
    return adc >> 4;   // 0..4095 -> 0..255
}
