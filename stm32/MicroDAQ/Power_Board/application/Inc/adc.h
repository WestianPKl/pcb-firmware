#ifndef ADC_H
#define ADC_H

#include <stdint.h>
#include "stm32c0xx.h"

#define VIN_PIN 0U // PA0
#define V12_PIN 1U // PA1
#define V5_PIN 2U // PA2
#define V33_PIN 3U // PA3

#define ADC_CHANNEL_COUNT 4U

#define VDDA_MV 3300U

#define VIN_MIN_MV   11500U
#define VIN_MAX_MV   12500U

#define V12_MIN_MV   11500U
#define V12_MAX_MV   12200U

#define V5_MIN_MV     4500U
#define V5_MAX_MV     5500U

#define V33_MIN_MV    3000U
#define V33_MAX_MV    3500U

extern volatile uint16_t adc_buffer[ADC_CHANNEL_COUNT];
extern volatile uint16_t adc_vin;
extern volatile uint16_t adc_v12;
extern volatile uint16_t adc_v5;
extern volatile uint16_t adc_v33;
extern volatile uint8_t adc_data_valid;

void adc1_init(void);
void adc1_start(void);
void adc1_dma_init(void);
void adc_update(void);

#endif // ADC_H