#pragma once
#include <stdint.h>

// one PCNT unit in x4 quadrature decode: both edges of both channels counted,
// so direction comes out of the A/B phase relation instead of a separate pin
void pcnt_encoder_init(void);
int pcnt_encoder_get_count(void);
