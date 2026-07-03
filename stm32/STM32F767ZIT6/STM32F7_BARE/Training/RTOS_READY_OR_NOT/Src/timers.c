#include "timers.h"
#include "stm32f767xx.h"
#include "support.h"

volatile uint32_t tick_10ms = 0;
volatile uint8_t clock_tick_flag = 0;

void init_timer_tim2_ch1(void)
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

void init_timer_tim4_ch2_pwm(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(0x3U << (LED2 * 2U));
    GPIOB->MODER |=  (0x2U << (LED2 * 2U)); 
    GPIOB->PUPDR &= ~(0x3U << (LED2 * 2U));
    GPIOB->OTYPER &= ~(1U << LED2);
    GPIOB->OSPEEDR &= ~(0x3U << (LED2 * 2U));
    GPIOB->OSPEEDR |=  (0x2U << (LED2 * 2U));

    GPIOB->AFR[0] &= ~(0xFU << (7U * 4U));
    GPIOB->AFR[0] |=  (0x2U << (7U * 4U));

    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;

    TIM4->CR1 = 0;
    TIM4->CR2 = 0;

    timclk = tim_apb1_clock_hz();
    
    psc = (timclk / (1000U * 100U));
    if (psc == 0U) {
        psc = 1U;
    }

    TIM4->PSC = psc - 1U;

    arr = 100U;
    TIM4->ARR = arr - 1U;

    TIM4->CCMR1 &= ~(TIM_CCMR1_CC2S_Msk | TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2PE);
    TIM4->CCMR1 |= TIM_CCMR1_OC2M_1 | TIM_CCMR1_OC2M_2 | TIM_CCMR1_OC2PE;

    TIM4->CCER &= ~(TIM_CCER_CC2P | TIM_CCER_CC2NP);
    TIM4->CCER |= TIM_CCER_CC2E;

    TIM4->CCR2 = 0U;
    TIM4->CNT = 0U;

    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR = TIM_EGR_UG;
    TIM4->CR1 |= TIM_CR1_CEN;
}

void set_duty_timer_tim4_ch2_pwm(uint8_t duty_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    arr = TIM4->ARR;
    ccr = ((uint32_t)duty_percent * (arr + 1U)) / 100U;

    if (ccr > arr) {
        ccr = arr;
    }

    TIM4->CCR2 = ccr;
}



void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;
        tick_10ms++;

        if (tick_10ms % 100U == 0U) {
            tick_10ms = 0;
            clock_tick_flag = 1;
        }
    }
}