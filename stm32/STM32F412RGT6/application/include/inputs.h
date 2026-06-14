#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>
#include "stm32f4xx.h"

void esp32_status_init(void);
uint8_t esp32_status_get(void);

void btn1_irq_init(void);
void btn2_irq_init(void);
void mfp_irq_init(void);

#endif // INPUTS_H