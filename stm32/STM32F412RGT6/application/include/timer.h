#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "stm32f4xx.h"

void timer1_init(uint32_t prescaler, uint32_t period);
void timer2_init(uint32_t prescaler, uint32_t period);
void timer3_init(uint32_t prescaler, uint32_t period);
void timer4_init(uint32_t prescaler, uint32_t period);
void timer13_init_10ms(void);

void timer1_pwm_ch1_init(uint32_t prescaler, uint32_t period);

void timer2_pwm_ch3_init(uint32_t prescaler, uint32_t period);

void timer3_pwm_ch1_init(uint32_t prescaler, uint32_t period);
void timer3_pwm_ch2_init(uint32_t prescaler, uint32_t period);
void timer3_pwm_ch3_init(uint32_t prescaler, uint32_t period);
void timer8_pwm_ch4_init(uint32_t prescaler, uint32_t period);

void timer4_pwm_ch3_init(uint32_t prescaler, uint32_t period);
void timer4_pwm_ch4_init(uint32_t prescaler, uint32_t period);

void timer1_pwm_ch1_set_duty(uint8_t duty_0_255);
void timer2_pwm_ch3_set_duty(uint8_t duty_0_255);
void timer4_pwm_ch3_set_duty(uint8_t duty_0_255);
void timer4_pwm_ch4_set_duty(uint8_t duty_0_255);

void timer3_pwm_ch1_set_duty(int32_t duty);
void timer3_pwm_ch2_set_duty(int32_t duty);
void timer3_pwm_ch3_set_duty(int32_t duty);
void timer8_pwm_ch4_set_duty(int32_t duty);

void timer3_pwm_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness);

void timer8_pwm_set_buzzer_freq(uint32_t freq, uint32_t volume);

#endif // TIMER_H