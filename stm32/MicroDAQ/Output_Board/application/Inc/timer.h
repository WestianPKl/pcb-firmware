#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "stm32c5xx.h"

extern volatile uint32_t tick_100ms;
extern volatile uint32_t tick_1s;
extern volatile uint8_t clock_tick_flag;
extern volatile uint8_t clock_tick_100ms_flag;

void tim6_init(void);

#endif // TIMER_H