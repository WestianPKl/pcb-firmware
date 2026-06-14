#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include "stm32f4xx.h"

void systick_delay_ms(uint32_t ms);

#endif // SYSTICK_H