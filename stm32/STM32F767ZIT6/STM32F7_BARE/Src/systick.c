#include "systick.h"

static volatile uint32_t systick_ms_counter = 0;

void SysTick_Handler(void)
{
    systick_ms_counter++;
}

void systick_init(void)
{
    SysTick->LOAD = (SystemCoreClock / 1000U) - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk
                  | SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk;
}

uint32_t systick_get_ms(void)
{
    return systick_ms_counter;
}

void systick_delay_ms(uint32_t ms)
{
    if (ms == 0U) return;

    if (__get_PRIMASK() == 0U) {
        uint32_t start = systick_ms_counter;
        while ((systick_ms_counter - start) < ms) {}
        return;
    }

    uint32_t ticks_per_ms = SystemCoreClock / 1000U;

    while (ms--) {
        SysTick->LOAD = ticks_per_ms - 1U;
        SysTick->VAL  = 0U;
        SysTick->CTRL = SysTick_CTRL_ENABLE_Msk | SysTick_CTRL_CLKSOURCE_Msk;
        while (!(SysTick->CTRL & SysTick_CTRL_COUNTFLAG_Msk)) {}
    }
    SysTick->LOAD = ticks_per_ms - 1U;
    SysTick->VAL  = 0U;
    SysTick->CTRL = SysTick_CTRL_ENABLE_Msk
                  | SysTick_CTRL_CLKSOURCE_Msk
                  | SysTick_CTRL_TICKINT_Msk;
}