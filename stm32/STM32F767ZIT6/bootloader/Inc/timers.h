#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include "stm32f7xx.h"


extern volatile uint32_t tick_10ms;
extern volatile uint32_t tick_100ms;
extern volatile uint8_t clock_tick_flag;

void init_timer_tim2_ch1(void);

#endif // TIMERS_H