#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "stm32f7xx.h"

#define LED2  7U // LED2 PB7 - TIM4 CH2

extern volatile uint32_t tick_10ms;
extern volatile uint32_t tick_100ms;
extern volatile uint8_t clock_tick_flag;

void init_timer_tim2_ch1(void);

void init_timer_tim4_ch2_pwm(void);
void set_duty_timer_tim4_ch2_pwm(uint8_t duty_percent);

#endif // TIMER_H