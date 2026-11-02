#include "support.h"

static const uint8_t  apb_presc_table[8]  = { 1, 1, 1, 1, 2, 4, 8, 16 };
// static const uint16_t ahb_presc_table[16] = {
//     1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 8, 16, 64, 128, 256, 512
// };



static uint32_t apb1_div_get(void)
{
    uint32_t ppre1_bits = (RCC->CFGR2 >> 4U) & 0x07U;
    return apb_presc_table[ppre1_bits];
}

uint32_t apb1_pclk_hz(void)
{
    // uint32_t hpre_bits = (RCC->CFGR2 >> 0U) & 0x0FU;
    // uint32_t hclk = SystemCoreClock / ahb_presc_table[hpre_bits];
    uint32_t hclk = SystemCoreClock;
    return hclk / apb1_div_get();
}

uint32_t tim_apb1_clock_hz(void)
{
    uint32_t pclk1 = apb1_pclk_hz();
    uint32_t div   = apb1_div_get();
    uint8_t  multiplier;

    if ((RCC->CFGR1 & (0x1UL << 15U)) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? 2U : 4U;
    }
    return pclk1 * multiplier;
}

void clock_init_250mhz(void)
{
    RCC->CR &= ~(RCC_CR_HSIDIV_Msk);
    RCC->CR |= (0x1U << RCC_CR_HSIDIV_Pos);
    while ((RCC->CR & RCC_CR_HSIDIVF_Msk) == 0U){}

    PWR->VOSCR &= ~(PWR_VOSCR_VOS_Msk);
    PWR->VOSCR |= (0x3U << PWR_VOSCR_VOS_Pos);
    while (!(PWR->VOSSR & (0x1U << PWR_VOSSR_VOSRDY_Pos))) {}

    FLASH->ACR &= ~(FLASH_ACR_LATENCY_Msk);
    FLASH->ACR |= (5U << FLASH_ACR_LATENCY_Pos);

    FLASH->ACR &= ~(FLASH_ACR_WRHIGHFREQ_Msk);
    FLASH->ACR |= (0x2U << FLASH_ACR_WRHIGHFREQ_Pos);

    RCC->CR &= ~( RCC_CR_PLL1ON_Msk);
    while((RCC->CR & (0x1U << RCC_CR_PLL1RDY_Pos)) != 0U) {}

    RCC->PLL1CFGR &= ~(RCC_PLL1CFGR_PLL1SRC_Msk);
    RCC->PLL1CFGR |= (1U << RCC_PLL1CFGR_PLL1SRC_Pos);

    RCC->PLL1CFGR &= ~(RCC_PLL1CFGR_PLL1M_Msk);
    RCC->PLL1CFGR |= (8U << RCC_PLL1CFGR_PLL1M_Pos);

    RCC->PLL1CFGR &= ~(RCC_PLL1CFGR_PLL1RGE_Msk);
    RCC->PLL1CFGR |= (0x1U << RCC_PLL1CFGR_PLL1RGE_Pos);

    RCC->PLL1CFGR &= ~(RCC_PLL1CFGR_PLL1VCOSEL_Msk);
    RCC->PLL1CFGR &= ~(RCC_PLL1CFGR_PLL1FRACEN_Msk);

    RCC->PLL1DIVR &= ~(RCC_PLL1DIVR_PLL1N_Msk);
    RCC->PLL1DIVR |= (124U << RCC_PLL1DIVR_PLL1N_Pos);

    RCC->PLL1DIVR &= ~(RCC_PLL1DIVR_PLL1P_Msk);
    RCC->PLL1DIVR |= (1U << RCC_PLL1DIVR_PLL1P_Pos);

    RCC->PLL1CFGR |= RCC_PLL1CFGR_PLL1PEN;

    RCC->CR |= (0x1UL << RCC_CR_PLL1ON_Pos);
    while((RCC->CR & (0x1U << RCC_CR_PLL1RDY_Pos)) == 0U) {}

    RCC->CFGR1 &= ~(RCC_CFGR1_SW_Msk);
    RCC->CFGR1 |= (0x3U << RCC_CFGR1_SW_Pos);
    while ((RCC->CFGR1 & RCC_CFGR1_SWS_Msk) != (0x3U << RCC_CFGR1_SWS_Pos)) {}

    SystemCoreClockUpdate();
}