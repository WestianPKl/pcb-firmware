#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdint.h>
#include "stm32f4xx.h"

void portc_init(void);
void portb_init(void);

void pin_set_high(uint8_t port, uint8_t pin);
void pin_set_low(uint8_t port, uint8_t pin);

void esp32_status_set(uint8_t status);
void esp32_reset(void);

#endif // OUTPUTS_H