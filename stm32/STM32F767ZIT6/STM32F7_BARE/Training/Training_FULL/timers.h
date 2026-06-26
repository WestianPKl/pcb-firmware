#ifndef TIMERS_H
#define TIMERS_H

#include <stdint.h>
#include "stm32f7xx.h"

void init_timer_TIM2_CH1(void);
void init_timer_TIM1_1hz(void);

#endif // TIMERS_H