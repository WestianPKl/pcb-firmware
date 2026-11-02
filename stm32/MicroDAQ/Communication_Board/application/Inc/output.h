#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include "stm32c5xx.h"

#define CAN_TERM_PIN 13U // PC13
#define RS485_TERM_PIN 14U // PC14

void gpio_output_init(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_off(GPIO_TypeDef *GPIOx, uint32_t pin);

#endif // OUTPUT_H

