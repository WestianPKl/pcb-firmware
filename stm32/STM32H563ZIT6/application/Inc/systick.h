#ifndef SYSTICK_H
#define SYSTICK_H

#include <stdint.h>
#include "stm32h5xx.h"

uint8_t systick_init(uint32_t tick_hz);

#endif // SYSTICK_H
