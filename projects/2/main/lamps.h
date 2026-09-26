#pragma once

typedef enum { LAMP_OFF, LAMP_ON, LAMP_BLINK } lamp_mode_t;

void lamps_init(void);
void lamps_set(lamp_mode_t red, lamp_mode_t yellow, lamp_mode_t green);
