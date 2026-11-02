#include "timer.h"
#include "support.h"

volatile uint32_t tick_100ms = 0U;
volatile uint32_t tick_1s = 0U;
volatile uint8_t clock_tick_flag = 0U;
volatile uint8_t clock_tick_100ms_flag = 0U;

void tim3_init(void)
{
    uint32_t timclk;
    uint32_t psc;

    RCC->APBENR1 |= RCC_APBENR1_TIM3EN;
    (void)RCC->APBENR1;

    TIM3->CR1 = 0U;
    TIM3->CR2 = 0U;

    timclk = tim_apb_clock_hz();
    
    psc = timclk / 1000U;

    if (psc == 0U)
    {
        psc = 1U;
    }

    TIM3->PSC = psc - 1U;

    TIM3->ARR = 10U - 1U;

    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR = 0U;

    TIM3->DIER |= TIM_DIER_UIE;

    NVIC_SetPriority(TIM3_IRQn, 2U);
    NVIC_ClearPendingIRQ(TIM3_IRQn);
    NVIC_EnableIRQ(TIM3_IRQn);

    TIM3->CR1 |= TIM_CR1_CEN;
}

void TIM3_IRQHandler(void)
{
    if ((TIM3->SR & TIM_SR_UIF) != 0U)
    {
        TIM3->SR = ~TIM_SR_UIF;

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