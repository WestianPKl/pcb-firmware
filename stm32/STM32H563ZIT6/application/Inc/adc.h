#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "stm32h5xx.h"

#define ADC1_PIN 0U // PA0
#define ADC1_CH0  0U // ADC1_IN0

extern volatile uint16_t adc_buffer[100];

void adc1_init(void);
void adc1_trigg_init(void);
void adc1_dma_init(void);
uint8_t adc1_read(uint16_t *value);
uint8_t adc1_trigg_read(uint16_t *value);
uint8_t adc1_dma_start(void);

#endif // ADC_H
