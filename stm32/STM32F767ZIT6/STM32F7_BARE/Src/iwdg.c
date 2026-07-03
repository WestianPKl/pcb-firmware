#include "iwdg.h"

void iwdg_init(void)
{
    IWDG->KR  = 0x5555U;
    IWDG->PR  = 0x1U;
    IWDG->RLR = 0xFA0U;

    uint32_t t = 100000U;
    while ((IWDG->SR & IWDG_SR_PVU) && --t);
    t = 100000U;
    while ((IWDG->SR & IWDG_SR_RVU) && --t);

    IWDG->KR  = 0xAAAAU;
    IWDG->KR  = 0xCCCCU;
}

void iwdg_refresh(void)
{
    IWDG->KR = 0xAAAAU;
}