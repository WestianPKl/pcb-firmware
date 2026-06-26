#include "timers.h"
#include "support.h"


void init_timer_TIM1_1hz(void)
{
    uint32_t timclk;
    uint32_t psc;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN; // enable TIM1 clock
    (void)RCC->APB2ENR; // delay after enabling clock

    TIM1->CR1 = 0; // clear control register 1
    TIM1->CR2 = 0; // clear control register 2

    timclk = tim_apb2_clock_hz();

    psc = (timclk / 10000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM1->PSC = psc - 1U; // set prescaler for 10kHz timer clock
    TIM1->ARR = 10000U - 1U; // set auto-reload value for 1 second period

    TIM1->SR = 0; // clear status register

    TIM1->CR1 |= TIM_CR1_CEN; // enable TIM1
}


void init_timer_TIM2_CH1(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN; // enable TIM2 clock
    (void)RCC->APB1ENR; // delay after enabling clock

    TIM2->CR1 = 0; // clear control register 1
    TIM2->CR2 = 0; // clear control register 2

    timclk = tim_apb1_clock_hz();
    
    psc = (timclk / 10000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM2->PSC = psc - 1U; // set prescaler for 10kHz timer clock

    arr = 100U; // set auto-reload value for 10ms period
    TIM2->ARR = arr - 1U; // set auto-reload value for 10ms period

    TIM2->EGR = TIM_EGR_UG; // generate an update event to load the prescaler value
    TIM2->SR = 0; // clear status register
    TIM2->DIER |= TIM_DIER_UIE; // enable update interrupt

    NVIC_SetPriority(TIM2_IRQn, 1); // set priority for TIM2 interrupt
    NVIC_EnableIRQ(TIM2_IRQn); // enable TIM2 interrupt in NVIC
    TIM2->CR1 |= TIM_CR1_CEN; // enable TIM2
}
