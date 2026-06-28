#include "iwdg.h"

void iwdg_init(void)
{
    IWDG->KR  = 0x5555U;
    IWDG->PR  = 0x1U;
    IWDG->RLR = 0xFA0U;

    while (IWDG->SR & IWDG_SR_PVU) {}
    while (IWDG->SR & IWDG_SR_RVU) {}

    IWDG->KR  = 0xAAAAU;
    IWDG->KR  = 0xCCCCU;
}

void iwdg_refresh(void)
{
    IWDG->KR = 0xAAAAU;
}