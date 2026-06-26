#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "stm32f7xx.h"

#define ADC1_CH4 4U // ADC1 channel 4 (PA4)

void adc_init_PA4_continuous(void);
void adc_init_PA4_single(void);
void adc_init_PA4_interrupt(void);
uint32_t adc_read_PA4(void);
void start_adc_conversion_PA4(void);
uint32_t adc_read_PA4(void);

#endif // ADC_H