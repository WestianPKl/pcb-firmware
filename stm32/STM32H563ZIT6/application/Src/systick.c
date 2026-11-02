#include "systick.h"

volatile uint32_t time_systick_ms = 0U;

uint8_t systick_init(uint32_t tick_hz)
{
    if (tick_hz == 0U)
    {
        return 0U;
    }

    uint32_t reload = SystemCoreClock / tick_hz;

    if ((reload == 0U) || (reload > 0x01000000U))
    {
        return 0U;
    }

    SysTick->CTRL = 0U;

    SysTick->LOAD = reload - 1U;

    SysTick->VAL = 0U;

    SysTick->CTRL =
        SysTick_CTRL_CLKSOURCE_Msk |
        SysTick_CTRL_TICKINT_Msk |
        SysTick_CTRL_ENABLE_Msk;

    return 1U;
}

void SysTick_Handler(void)
{
    time_systick_ms++;
}