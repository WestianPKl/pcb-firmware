#include "support.h"
#include "timer.h"

static const uint8_t  apb_presc_table[8]  = { 1, 1, 1, 1, 2, 4, 8, 16 };

static uint32_t apb1_div_get(void)
{
    uint32_t ppre1_bits = (RCC->CFGR2 >> 4U) & 0x07U;
    return apb_presc_table[ppre1_bits];
}

static uint32_t apb2_div_get(void)
{
    uint32_t ppre2_bits = (RCC->CFGR2 >> 8U) & 0x07U;
    return apb_presc_table[ppre2_bits];
}

uint32_t apb1_pclk_hz(void)
{
    uint32_t hclk = SystemCoreClock;
    return hclk / apb1_div_get();
}

uint32_t apb2_pclk_hz(void)
{
    uint32_t hclk = SystemCoreClock;
    return hclk / apb2_div_get();
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

uint32_t tim_apb2_clock_hz(void)
{
    uint32_t pclk2 = apb2_pclk_hz();
    uint32_t div   = apb2_div_get();
    uint8_t  multiplier;

    if ((RCC->CFGR1 & (0x1UL << 15U)) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? 2U : 4U;
    }
    return pclk2 * multiplier;
}