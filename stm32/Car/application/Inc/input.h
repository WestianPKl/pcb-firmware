#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "stm32f7xx.h"

#define DRV8876_FAULT_Pin 8U // PC8

extern volatile uint8_t drv8876_fault_flag;
extern volatile uint32_t drv8876_fault_counter;

void gpio_input_init(GPIO_TypeDef *GPIOx, uint32_t pin);

#endif // INPUT_H