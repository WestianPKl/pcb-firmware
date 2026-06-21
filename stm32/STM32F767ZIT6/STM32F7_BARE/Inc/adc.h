#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "stm32f7xx.h"

#define ADC1_CH4 4U // ADC1 channel 4 (PA4)

void adc_init(void);
uint32_t adc_read(void);

#endif // ADC_H