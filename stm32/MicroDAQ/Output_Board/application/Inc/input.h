
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "stm32c5xx.h"

#define SLOT_ID0_PIN 12U // PB12
#define SLOT_ID1_PIN 13U // PB13

extern volatile uint8_t slot_id;

void gpio_input_init(GPIO_TypeDef *GPIOx, uint32_t pin);
void input_init(void);
void slot_id_read(void);

#endif // INPUT_H