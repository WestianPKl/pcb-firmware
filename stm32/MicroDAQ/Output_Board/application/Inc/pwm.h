#ifndef PWM_H
#define PWM_H

#include <stdint.h>
#include "stm32c5xx.h"

#define PWM_PHYSICAL_COUNT 4U
#define PWM_DUTY_MAX       100U

#define PWM_FREQUENCY_HZ 1000U
#define PWM_PERIOD_COUNTS 1000U

#define PWM0_PIN 8U //PA8 TIM1_CH1
#define PWM1_PIN 9U //PA9 TIM1_CH2
#define PWM2_PIN 10U //PA10 TIM1_CH3
#define PWM3_PIN 15U //PA15 TIM2_CH1

typedef struct {
    uint8_t pwm0;
    uint8_t pwm1;
    uint8_t pwm2;
    uint8_t pwm3;
} pwm_status_t;

typedef struct
{
    uint8_t channel;
    uint8_t duty_cycle;
} pwm_config_t;

static const pwm_config_t pwm_config[] =
{
    {PWM0_PIN, 0U},
    {PWM1_PIN, 0U},
    {PWM2_PIN, 0U},
    {PWM3_PIN, 0U},
};

extern volatile uint8_t pwm_max;

void pwm_init(void);
uint8_t pwm_set_state(uint8_t pwm_number, uint8_t duty_cycle);
uint8_t pwm_get_state(uint8_t pwm_number);
void pwm_get_status(pwm_status_t *status);
uint16_t pwm_get_gpio_mask(void);
uint16_t pwm_get_mask(void);
uint8_t pwm_get_duty(uint8_t pwm_number);

#endif // PWM_H