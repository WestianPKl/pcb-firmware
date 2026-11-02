#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include "stm32c0xx.h"

void gpio_output_init(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_off(GPIO_TypeDef *GPIOx, uint32_t pin);

#endif // OUTPUT_H

