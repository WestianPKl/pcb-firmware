#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "stm32f7xx.h"

#define Enkoder_A_Pin 7U // TIM3_CH1 PC7
#define Enkoder_B_Pin 6U // TIM3_CH2 PC6
#define DRV8876_EN_Pin 9U // TIM1_CH1 PE9 
#define MG90S_PWM_Pin 12U //TIM4_CH1 PD12

extern volatile uint32_t tick_10ms;
extern volatile uint8_t clock_tick_flag;

void tim2_init(void);
void encoder_init(void);
float encoder_get_rpm(void);
void servo_pwm_init(void);
void servo_set_us(uint16_t pulse_us);
void motor_pwm_init(void);
void motor_pwm_set(uint16_t duty);


#endif // TIMER_H