#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdint.h>
#include "stm32f7xx.h"

#define LED1  0U // LED1 PB0
#define LED3  14U // LED3 PB14

void init_leds(void);

#endif // OUTPUTS_H