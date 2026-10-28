#ifndef ADC_H
#define ADC_H

typedef struct {
    int     buf[16];
    int     idx;
    int     count;
    int32_t sum;
} sma_t;

void setup_adc(void);
int adc_read(void);
sma_t setup_averaging(void);
int sma_update(sma_t *s, int x);
void warm_up(sma_t *s);

#endif
