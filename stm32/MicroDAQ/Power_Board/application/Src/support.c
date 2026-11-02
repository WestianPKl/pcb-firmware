#include <stddef.h>
#include "support.h"
#include "timer.h"

static const uint8_t  apb_presc_table[8]  = { 1, 1, 1, 1, 2, 4, 8, 16 };

static uint32_t apb_div_get(void)
{
    uint32_t ppre_bits =
        (RCC->CFGR & RCC_CFGR_PPRE_Msk) >> RCC_CFGR_PPRE_Pos;

    return apb_presc_table[ppre_bits];
}

uint32_t apb_pclk_hz(void)
{
    return SystemCoreClock / apb_div_get();
}

uint32_t tim_apb_clock_hz(void)
{
    uint32_t pclk = apb_pclk_hz();
    uint32_t div  = apb_div_get();

    return (div == 1U) ? pclk : (pclk * 2U);
}

void reset_reason_get(reset_reason_t *r)
{
    uint32_t reason = RCC->CSR2;

    if (r != NULL) {
        r->reset_pin = ((reason & RCC_CSR2_PINRSTF)  != 0U);
        r->reset_bor = ((reason & RCC_CSR2_PWRRSTF) != 0U);;
        r->reset_soft = ((reason & RCC_CSR2_SFTRSTF)  != 0U);
        r->reset_iwdg = ((reason & RCC_CSR2_IWDGRSTF) != 0U);
        r->reset_wwdg = ((reason & RCC_CSR2_WWDGRSTF) != 0U);
        r->reset_low_power = ((reason & RCC_CSR2_LPWRRSTF) != 0U);
    }

    RCC->CSR2 |= RCC_CSR2_RMVF;
}

uint8_t reset_reason_pack(const reset_reason_t *r)
{
    uint8_t value = 0U;

    if (r->reset_pin)       value |= RESET_REASON_PIN;
    if (r->reset_bor)       value |= RESET_REASON_BOR;
    if (r->reset_soft)      value |= RESET_REASON_SOFT;
    if (r->reset_iwdg)      value |= RESET_REASON_IWDG;
    if (r->reset_wwdg)      value |= RESET_REASON_WWDG;
    if (r->reset_low_power) value |= RESET_REASON_LOW_POWER;

    return value;
}

uint8_t iwdg_init(void)
{
    IWDG->KR = 0xCCCCU;
    IWDG->KR = 0x5555U;
    IWDG->PR  = 3U;
    IWDG->RLR = 1999U;

    uint32_t timeout = 100000U;

    while ((IWDG->SR & (IWDG_SR_PVU | IWDG_SR_RVU)) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    IWDG->KR = 0xAAAAU;

    return 1U;
}

void iwdg_refresh(void)
{
    IWDG->KR = 0xAAAAU;
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

uint32_t crc32(const uint8_t *data, uint32_t len)
{
    if (data == NULL)
    {
        return 0U;
    }

    uint32_t crc = 0xFFFFFFFFU;
    for (uint32_t i = 0U; i < len; i++)
    {
        crc ^= data[i];
        for (uint8_t b = 0U; b < 8U; b++){
            if ((crc & 1U) != 0U){
                crc = (crc >> 1U) ^ 0xEDB88320U;
            }
            else{
                crc >>= 1U;
            }
        }
    }
    return crc ^ 0xFFFFFFFFU;
}