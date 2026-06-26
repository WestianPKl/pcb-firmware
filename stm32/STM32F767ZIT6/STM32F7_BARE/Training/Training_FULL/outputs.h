#ifndef OUTPUTS_H
#define OUTPUTS_H

#include <stdint.h>
#include "stm32f7xx.h"

// LED1 PB0
// LED2 PB7
// LED3 PB14
#define LED_1  (1U << 0)
#define LED_2  (1U << 7)
#define LED_3  (1U << 14)

void init_leds(void);


#endif // OUTPUTS_H