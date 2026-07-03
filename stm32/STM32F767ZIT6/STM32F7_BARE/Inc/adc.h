#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "stm32f7xx.h"

#define ADC1_CH4 4U // ADC1 channel 4 PA4

#define ADC_BUFFER_SIZE 1

extern uint16_t adc_data_buffer[ADC_BUFFER_SIZE];

void adc1_ch4_continuous(uint8_t continuous);
void adc1_start_conversion(void);
void adc1_stop_conversion(void);
uint8_t adc1_is_conversion_complete(void);
void adc1_clear_complete_flag(void);

#endif // ADC_H