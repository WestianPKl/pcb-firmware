#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include "stm32f7xx.h"

#define DRV8876_PH_Pin 13U // GPIO PF13
#define DRV8876_nSLEEP_Pin 14U // GPIO PF14

#define LED1  0U // LED1 PB0
#define LED3  14U // LED3 PB14

void gpio_output_init(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_off(GPIO_TypeDef *GPIOx, uint32_t pin);

#endif // OUTPUT_H