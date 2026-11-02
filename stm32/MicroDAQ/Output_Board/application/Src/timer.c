#include "timer.h"
#include "support.h"

volatile uint32_t tick_100ms = 0U;
volatile uint32_t tick_1s = 0U;
volatile uint8_t clock_tick_flag = 0U;
volatile uint8_t clock_tick_100ms_flag = 0U;

void tim6_init(void)
{
    uint32_t timclk;
    uint32_t psc;

    RCC->APB1LENR |= RCC_APB1LENR_TIM6EN;
    (void)RCC->APB1LENR;

    TIM6->CR1 = 0U;
    TIM6->CR2 = 0U;

    timclk = tim_apb1_clock_hz();
    
    psc = timclk / 1000U;

    if (psc == 0U)
    {
        psc = 1U;
    }

    TIM6->PSC = psc - 1U;

    TIM6->ARR = 10U - 1U;

    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0U;

    TIM6->DIER |= TIM_DIER_UIE;

    NVIC_ClearPendingIRQ(TIM6_IRQn);
    NVIC_SetPriority(TIM6_IRQn, 2U);
    NVIC_EnableIRQ(TIM6_IRQn);

    TIM6->CR1 |= TIM_CR1_CEN;
}

void TIM6_IRQHandler(void)
{
    if ((TIM6->SR & TIM_SR_UIF) != 0U)
    {
        TIM6->SR = ~TIM_SR_UIF;

        if (++tick_100ms >= 10U)
        {
            tick_100ms = 0U;
            clock_tick_100ms_flag = 1U;
        }

        if (++tick_1s >= 100U)
        {
            tick_1s = 0U;
            clock_tick_flag = 1U;
        }
    }
}