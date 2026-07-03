#include "support.h"

static uint32_t apb1_div_get(void)
{
    static const uint8_t apb_presc_table[8] = { 1, 1, 1, 1, 2, 4, 8, 16 };
    uint32_t ppre1_bits = (RCC->CFGR >> RCC_CFGR_PPRE1_Pos) & 0x07U;
    return apb_presc_table[ppre1_bits];
}

uint32_t apb1_pclk_hz(void)
{
    static const uint16_t ahb_presc_table[16] = {
        1,1,1,1,1,1,1,1, 2,4,8,16,64,128,256,512
    };
    uint32_t hpre_bits = (RCC->CFGR >> RCC_CFGR_HPRE_Pos) & 0x0FU;
    uint32_t hclk = SystemCoreClock / ahb_presc_table[hpre_bits];
    return hclk / apb1_div_get();
}

uint32_t tim_apb1_clock_hz(void)
{
    uint32_t pclk1 = apb1_pclk_hz();
    uint32_t div   = apb1_div_get();
    uint8_t  multiplier;

    if ((RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? (uint8_t)div : 4U;
    }
    return pclk1 * multiplier;
}

static uint32_t apb2_div_get(void)
{
    static const uint8_t apb_presc_table[8] = { 1, 1, 1, 1, 2, 4, 8, 16 };
    uint32_t ppre2_bits = (RCC->CFGR >> RCC_CFGR_PPRE2_Pos) & 0x07U;
    return apb_presc_table[ppre2_bits];
}

uint32_t apb2_pclk_hz(void)
{
    static const uint16_t ahb_presc_table[16] = {
        1,1,1,1,1,1,1,1, 2,4,8,16,64,128,256,512
    };
    uint32_t hpre_bits = (RCC->CFGR >> RCC_CFGR_HPRE_Pos) & 0x0FU;
    uint32_t hclk = SystemCoreClock / ahb_presc_table[hpre_bits];
    return hclk / apb2_div_get();
}

uint32_t tim_apb2_clock_hz(void)
{
    uint32_t pclk2 = apb2_pclk_hz();
    uint32_t div   = apb2_div_get();
    uint8_t  multiplier;

    if ((RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? (uint8_t)div : 4U;
    }
    return pclk2 * multiplier;
}

reset_cause_t get_reset_cause(void)
{
    uint32_t csr = RCC->CSR;
    reset_cause_t cause;

    if      (csr & RCC_CSR_IWDGRSTF) cause = RESET_IWDG;
    else if (csr & RCC_CSR_WWDGRSTF) cause = RESET_WWDG;
    else if (csr & RCC_CSR_SFTRSTF)  cause = RESET_SOFTWARE;
    else if (csr & RCC_CSR_BORRSTF)  cause = RESET_BROWNOUT;
    else if (csr & RCC_CSR_PINRSTF)  cause = RESET_PIN;
    else if (csr & RCC_CSR_PORRSTF)  cause = RESET_POWER_ON;
    else                             cause = RESET_UNKNOWN;

    RCC->CSR |= RCC_CSR_RMVF;
    return cause;
}

