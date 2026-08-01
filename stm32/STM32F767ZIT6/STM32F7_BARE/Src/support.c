#include "support.h"
#include <stddef.h>

static const uint8_t  apb_presc_table[8]  = { 1, 1, 1, 1, 2, 4, 8, 16 };
static const uint16_t ahb_presc_table[16] = {
    1, 1, 1, 1, 1, 1, 1, 1, 2, 4, 8, 16, 64, 128, 256, 512
};

static uint32_t hclk_hz(void)
{
    uint32_t hpre_bits = (RCC->CFGR >> RCC_CFGR_HPRE_Pos) & 0x0FU;
    return SystemCoreClock / ahb_presc_table[hpre_bits];
}

static uint32_t apb_div_get(uint32_t ppre_pos)
{
    uint32_t ppre_bits = (RCC->CFGR >> ppre_pos) & 0x07U;
    return apb_presc_table[ppre_bits];
}

static uint32_t tim_clock_hz(uint32_t ppre_pos)
{
    uint32_t div  = apb_div_get(ppre_pos);
    uint32_t pclk = hclk_hz() / div;
    uint8_t  multiplier;

    if ((RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? (uint8_t)div : 4U;
    }
    return pclk * multiplier;
}

uint32_t apb1_pclk_hz(void)      { return hclk_hz() / apb_div_get(RCC_CFGR_PPRE1_Pos); }
uint32_t apb2_pclk_hz(void)      { return hclk_hz() / apb_div_get(RCC_CFGR_PPRE2_Pos); }
uint32_t tim_apb1_clock_hz(void) { return tim_clock_hz(RCC_CFGR_PPRE1_Pos); }
uint32_t tim_apb2_clock_hz(void) { return tim_clock_hz(RCC_CFGR_PPRE2_Pos); }

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

void dwt_init(void) {
    CoreDebug->DEMCR |= CoreDebug_DEMCR_TRCENA_Msk;
    DWT->LAR = 0xC5ACCE55U;
    DWT->CYCCNT = 0U;
    DWT->CTRL  |= DWT_CTRL_CYCCNTENA_Msk;
}

void delay_us(uint32_t us) {
    uint32_t start  = DWT->CYCCNT;
    uint32_t cycles = (SystemCoreClock / 1000000U) * us;
    while ((DWT->CYCCNT - start) < cycles) { }
}

void delay_ms(uint32_t ms) {
    while (ms--) {
        delay_us(1000U);
    }
}

uint8_t crc8_atm(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0x00;

    if (data == NULL) return 0;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

uint16_t crc16_ccitt(const uint8_t *data, uint32_t len)
{   
    uint16_t crc = 0xFFFF;

    if (data == NULL) return 0;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= (uint16_t)data[i] << 8;
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x8000)
                crc = (crc << 1) ^ 0x1021;
            else
                crc <<= 1;
        }
    }
    return crc;
}