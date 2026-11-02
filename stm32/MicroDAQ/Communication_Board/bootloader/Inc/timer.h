#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "stm32c5xx.h"

extern volatile uint32_t tick_10ms;
extern volatile uint8_t clock_tick_flag;

void tim2_init(void);

#endif // TIMER_H