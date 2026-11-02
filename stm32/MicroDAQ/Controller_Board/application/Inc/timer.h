
#ifndef TIMER_H
#define TIMER_H

#include <stdint.h>
#include "stm32h5xx.h"
#include "support.h"

#define PWM_PIN 6U //PC6
#define TIM4_CH1_PIN 6U //PB6

extern volatile uint32_t tim3_compare_count;
extern volatile uint8_t compare_1s_flag;
extern volatile uint32_t tick_10us;
extern volatile uint8_t clock_tick_flag_us;
extern volatile uint32_t tick_ms;
extern volatile uint8_t clock_tick_flag;
extern volatile uint8_t lptim_state;
extern volatile uint32_t lptim_seconds;

typedef struct
{
    uint32_t frequency_hz;
    uint32_t duty_percent;
} pwm_measurement_t;

void tim4_pwm_input_init(void);
uint8_t tim4_pwm_get_measurement(pwm_measurement_t *measurement);
void tim4_input_capture_init(void);
void tim3_ch1_compare_match_init(void);
void tim3_ch1_pwm_init(void);
void tim3_ch1_pwm_set_duty(uint8_t duty_percent);
void tim3_ch1_pwm_set_frequency(uint32_t frequency_hz);
void tim2_init(void);
void tim6_init(void);
void delay_10us(uint32_t ticks);
uint32_t millis(void);

uint8_t lptim1_init_1s(void);

#endif // TIMER_H
