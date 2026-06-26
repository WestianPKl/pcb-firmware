#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include "stm32f7xx.h"

uint32_t apb1_pclk_hz(void);
uint32_t tim_apb1_clock_hz(void);
uint32_t apb2_pclk_hz(void);
uint32_t tim_apb2_clock_hz(void);
void set_ahb1_periph_clock(uint32_t periph);
void set_ahb2_periph_clock(uint32_t periph);
void set_apb1_periph_clock(uint32_t periph);
void set_apb2_periph_clock(uint32_t periph);
void set_pin_mode(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode);

#endif // SUPPORT_H