#pragma once

void setup_adc(void);
int adc_read(void);            // raw 0..4095
int adc_read_mv(void);         // calibrated, millivolts
// this one can be useful, but not for this project
// int adc_raw_to_mv(int raw);    // convert a raw value (e.g. averaged)