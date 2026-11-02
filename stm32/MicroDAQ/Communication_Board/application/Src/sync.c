#include "sync.h"
#include "input.h"

volatile uint8_t sync_event = 0;
volatile uint32_t sync_counter = 0;

void sync_init(void)
{
    gpio_input_init(GPIOB, SYNC_PIN);

    EXTI->IMR1 &= ~(1UL << SYNC_PIN);

    EXTI->EXTICR[3] &= ~EXTI_EXTICR4_EXTI15_Msk;

    EXTI->RTSR1 |=  (1UL << SYNC_PIN);
    EXTI->FTSR1 &= ~(1UL << SYNC_PIN);

    EXTI->RPR1 = (1UL << SYNC_PIN);
    EXTI->FPR1 = (1UL << SYNC_PIN);

    NVIC_SetPriority(EXTI15_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI15_IRQn);
    NVIC_EnableIRQ(EXTI15_IRQn);

    EXTI->IMR1 |= (1UL << SYNC_PIN);
}

void EXTI15_IRQHandler(void)
{
    uint32_t mask = (1U << SYNC_PIN);

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;

        sync_counter++;
        sync_event = 1U;
    }
}