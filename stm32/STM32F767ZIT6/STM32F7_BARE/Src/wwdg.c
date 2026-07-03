#include "wwdg.h"


void wwdg_init(uint8_t prescaler, uint8_t window, uint8_t counter)
{
    RCC->APB1ENR |= RCC_APB1ENR_WWDGEN;
    (void)RCC->APB1ENR;

    WWDG->CFR = ((uint32_t)prescaler << WWDG_CFR_WDGTB_Pos) | (window & 0x7FU);
    WWDG->CR  = (counter & 0x7FU);
    WWDG->CR |= WWDG_CR_WDGA;
}

void wwdg_refresh(uint8_t counter)
{
    WWDG->CR = (counter & 0x7FU) | WWDG_CR_WDGA;
}