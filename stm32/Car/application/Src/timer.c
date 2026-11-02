#include "timer.h"
#include "support.h"
#include "output.h"

// #define MOTOR_PWM_ARR      21599U
#define MOTOR_PWM_ARR 53999U
#define ENCODER_PPR        8U
#define MOTOR_GEAR_RATIO   120U
#define ENCODER_MODE_MULT  4U

#define ENCODER_CPR (ENCODER_PPR * MOTOR_GEAR_RATIO * ENCODER_MODE_MULT)

volatile uint32_t tick_10ms = 0U;
volatile uint8_t clock_tick_flag = 0U;

void tim2_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    TIM2->CR1 = 0;
    TIM2->CR2 = 0;

    timclk = tim_apb1_clock_hz();
    
    psc = (timclk / 10000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM2->PSC = psc - 1U;

    arr = 100U;
    TIM2->ARR = arr - 1U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM2_IRQn, 1);
    NVIC_EnableIRQ(TIM2_IRQn);
    TIM2->CR1 |= TIM_CR1_CEN;
}

void motor_pwm_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN |
                    RCC_AHB1ENR_GPIOFEN;
    (void)RCC->AHB1ENR;

    output_off(GPIOF, DRV8876_PH_Pin);
    output_off(GPIOF, DRV8876_nSLEEP_Pin);

    gpio_output_init(GPIOF, DRV8876_PH_Pin);
    gpio_output_init(GPIOF, DRV8876_nSLEEP_Pin);

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    TIM1->CR1 = 0U;
    TIM1->CCER = 0U;
    TIM1->BDTR = 0U;

    // TIM1->PSC = 0U;
    // TIM1->ARR = MOTOR_PWM_ARR;
    // TIM1->CCR1 = 0U;

    TIM1->PSC = 1U;
    TIM1->ARR = MOTOR_PWM_ARR;
    TIM1->CCR1 = 0U;

    TIM1->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
    TIM1->CCER |= TIM_CCER_CC1E;
    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->SR = 0U;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1 |= TIM_CR1_CEN;

    GPIOE->OTYPER &= ~(1U << DRV8876_EN_Pin);
    GPIOE->OSPEEDR |= (3U << (DRV8876_EN_Pin * 2U));
    GPIOE->PUPDR &= ~(3U << (DRV8876_EN_Pin * 2U));

    GPIOE->AFR[1] &= ~(0xFU << 4U);
    GPIOE->AFR[1] |=  (1U << 4U);

    GPIOE->MODER &= ~(3U << (DRV8876_EN_Pin * 2U));
    GPIOE->MODER |=  (2U << (DRV8876_EN_Pin * 2U));
}

void motor_pwm_set(uint16_t duty)
{
    if (duty > 1000U)
    {
        duty = 1000U;
    }

    uint32_t compare = ((uint32_t)duty * (MOTOR_PWM_ARR + 1U)) / 1000U;
    TIM1->CCR1 = compare;
}

void encoder_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    GPIOC->MODER &= ~((3U << (Enkoder_A_Pin * 2U)) | (3U << (Enkoder_B_Pin * 2U)));
    GPIOC->MODER |=  ((2U << (Enkoder_A_Pin * 2U)) | (2U << (Enkoder_B_Pin * 2U)));
    GPIOC->PUPDR &= ~((3U << (Enkoder_A_Pin * 2U)) | (3U << (Enkoder_B_Pin * 2U)));
    GPIOC->AFR[0] &= ~((0xFU << (Enkoder_A_Pin * 4U)) | (0xFU << (Enkoder_B_Pin * 4U)));
    GPIOC->AFR[0] |=  ((2U << (Enkoder_A_Pin * 4U)) | (2U << (Enkoder_B_Pin * 4U)));

    TIM3->CR1 = 0U;
    TIM3->SMCR = 0U;
    TIM3->CCMR1 = 0U;
    TIM3->CCER = 0U;
    TIM3->PSC = 0U;
    TIM3->ARR = 0xFFFFU;
    TIM3->CCMR1 = (1U << TIM_CCMR1_CC1S_Pos) | (1U << TIM_CCMR1_CC2S_Pos) | (3U << TIM_CCMR1_IC1F_Pos) | (3U << TIM_CCMR1_IC2F_Pos);
    TIM3->SMCR = (3U << TIM_SMCR_SMS_Pos);
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CNT = 0U;
    TIM3->CCER = TIM_CCER_CC1E | TIM_CCER_CC2E;
    TIM3->CR1 |= TIM_CR1_CEN;
}

float encoder_get_rpm(void)
{
    static uint16_t previous_cnt = 0U;

    uint16_t current_cnt = (uint16_t)TIM3->CNT;
    int16_t delta = (int16_t)(uint16_t)(current_cnt - previous_cnt);

    previous_cnt = current_cnt;

    return ((float)delta * 6000.0f) / (float)ENCODER_CPR;
}

void servo_pwm_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;

    TIM4->CR1 = 0U;
    TIM4->CCER = 0U;

    TIM4->PSC  = 107U;
    TIM4->ARR  = 19999U;
    TIM4->CCR1 = 1500U;

    TIM4->CCMR1 = TIM_CCMR1_OC1M_2 | TIM_CCMR1_OC1M_1 | TIM_CCMR1_OC1PE;
    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR = TIM_EGR_UG;
    TIM4->SR = 0U;
    TIM4->CR1 |= TIM_CR1_CEN;

    GPIOD->OTYPER &= ~(1U << MG90S_PWM_Pin);
    GPIOD->OSPEEDR |= (3U << (MG90S_PWM_Pin * 2U));
    GPIOD->PUPDR &= ~(3U << (MG90S_PWM_Pin * 2U));

    GPIOD->AFR[1] &= ~(0xFU << 16U);
    GPIOD->AFR[1] |=  (2U << 16U);

    GPIOD->MODER &= ~(3U << (MG90S_PWM_Pin * 2U));
    GPIOD->MODER |=  (2U << (MG90S_PWM_Pin * 2U));
}

void servo_set_us(uint16_t pulse_us)
{
    if (pulse_us < 1000U) pulse_us = 1000U;
    if (pulse_us > 2000U) pulse_us = 2000U;

    TIM4->CCR1 = pulse_us;
}


void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR &= ~TIM_SR_UIF;

        tick_10ms++;

        if ((tick_10ms % 100U) == 0U)
        {
            clock_tick_flag = 1U;
        }
    }
}