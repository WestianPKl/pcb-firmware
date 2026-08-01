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


void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF;
        tick_10ms++;

        if (tick_10ms % 10U == 0U) {
            tick_10ms = 0;
            clock_tick_flag = 1;
        }
    }
}