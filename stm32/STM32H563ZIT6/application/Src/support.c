#include "support.h"
#include "timer.h"

#define WWDG_COUNTER   0x7FU
#define WWDG_WINDOW    0x60U
#define WWDG_PRESCALER 7U

#define DTS_LSE_FREQ 32768U
#define DTS_SAMPLING 15U


volatile uint8_t reset_pin = 0U;
volatile uint8_t reset_bor = 0U;
volatile uint8_t reset_soft = 0U;
volatile uint8_t reset_iwdg = 0U;
volatile uint8_t reset_wwdg = 0U;
volatile uint8_t reset_low_power = 0U;

volatile uint8_t wwdg_ewi_flag = 0U;

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

    /* SPI CLOCK */
    RCC->PLL1DIVR &= ~RCC_PLL1DIVR_PLL1Q_Msk;
    RCC->PLL1DIVR |= ((4U - 1U) << RCC_PLL1DIVR_PLL1Q_Pos);
    RCC->PLL1CFGR |= RCC_PLL1CFGR_PLL1QEN;
    /* SPI CLOCK */

    RCC->PLL1CFGR |= RCC_PLL1CFGR_PLL1PEN;

    RCC->CR |= (0x1UL << RCC_CR_PLL1ON_Pos);
    while((RCC->CR & (0x1U << RCC_CR_PLL1RDY_Pos)) == 0U) {}

    RCC->CFGR1 &= ~(RCC_CFGR1_SW_Msk);
    RCC->CFGR1 |= (0x3U << RCC_CFGR1_SW_Pos);
    while ((RCC->CFGR1 & RCC_CFGR1_SWS_Msk) != (0x3U << RCC_CFGR1_SWS_Pos)) {}

    SystemCoreClockUpdate();
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



void reset_reason_read(void)
{
    uint32_t rsr = RCC->RSR;

    reset_pin       = ((rsr & RCC_RSR_PINRSTF)  != 0U);
    reset_bor       = ((rsr & RCC_RSR_BORRSTF)  != 0U);
    reset_soft      = ((rsr & RCC_RSR_SFTRSTF)  != 0U);
    reset_iwdg      = ((rsr & RCC_RSR_IWDGRSTF) != 0U);
    reset_wwdg      = ((rsr & RCC_RSR_WWDGRSTF) != 0U);
    reset_low_power = ((rsr & RCC_RSR_LPWRRSTF) != 0U);
}

uint32_t reset_reason_get(void)
{
    uint32_t reason = RCC->RSR;

    RCC->RSR |= RCC_RSR_RMVF;

    return reason;
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

void wwdg_init(void)
{
    RCC->APB1LENR |= RCC_APB1LENR_WWDGEN;
    (void)RCC->APB1LENR;

    WWDG->CFR = ((WWDG_PRESCALER << WWDG_CFR_WDGTB_Pos) & WWDG_CFR_WDGTB_Msk) | ((WWDG_WINDOW << WWDG_CFR_W_Pos) & WWDG_CFR_W_Msk);

    WWDG->CFR |= WWDG_CFR_EWI;

    NVIC_SetPriority(WWDG_IRQn, 5U);
    NVIC_EnableIRQ(WWDG_IRQn);

    WWDG->CR = WWDG_CR_WDGA | ((WWDG_COUNTER << WWDG_CR_T_Pos) & WWDG_CR_T_Msk);
}

void wwdg_refresh(void)
{
    WWDG->CR = WWDG_CR_WDGA | ((WWDG_COUNTER << WWDG_CR_T_Pos) & WWDG_CR_T_Msk);
}

uint8_t wwdg_refresh_if_window(void)
{
    uint32_t counter =
        (WWDG->CR & WWDG_CR_T_Msk) >> WWDG_CR_T_Pos;

    if ((counter <= WWDG_WINDOW) && (counter >= 0x40U))
    {
        WWDG->CR = WWDG_CR_WDGA | ((WWDG_COUNTER << WWDG_CR_T_Pos) & WWDG_CR_T_Msk);

        return 1U;
    }

    return 0U;
}


void WWDG_IRQHandler(void)
{
    if ((WWDG->SR & WWDG_SR_EWIF) != 0U)
    {
        WWDG->SR = 0U;

        wwdg_ewi_flag = 1U;
    }
}

uint8_t rng_init(void)
{
    /* HSI48 ON */
    RCC->CR |= RCC_CR_HSI48ON;

    uint32_t timeout = 100000U;

    while ((RCC->CR & RCC_CR_HSI48RDY) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    /* RNGSEL = 00 -> HSI48 */
    RCC->CCIPR5 &= ~RCC_CCIPR5_RNGSEL_Msk;

    /* RNG bus clock */
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    (void)RCC->AHB2ENR;

    /* RNG off while configuring */
    RNG->CR &= ~RNG_CR_RNGEN;

    /*
     * Recommended NIST configuration provided
     * directly by STM32H563 CMSIS.
     */
    RNG->CR   = RNG_CR_NIST_VALUE | RNG_CR_CONDRST;
    RNG->NSCR = RNG_NSCR_NIST_VALUE;
    RNG->HTCR = RNG_HTCR_NIST_VALUE;

    /* Start conditioning */
    RNG->CR &= ~RNG_CR_CONDRST;

    timeout = 100000U;

    while ((RNG->CR & RNG_CR_CONDRST) != 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    /* Start RNG */
    RNG->CR |= RNG_CR_RNGEN;

    timeout = 100000U;

    while ((RNG->SR & RNG_SR_DRDY) == 0U)
    {
        if ((RNG->SR & RNG_SR_SECS) != 0U)
        {
            return 0U;
        }

        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    return 1U;
}

uint8_t rng_get(uint32_t *value)
{
    if (value == NULL)
    {
        return 0U;
    }

    uint32_t timeout = 100000U;

    while ((RNG->SR & RNG_SR_DRDY) == 0U)
    {
        if ((RNG->SR & (RNG_SR_SECS | RNG_SR_CECS)) != 0U)
        {
            return 0U;
        }

        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    *value = RNG->DR;

    return 1U;
}

void crc_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR;

    /* 32-bit polynomial, no input/output reversal */
    CRC->CR &= ~(CRC_CR_POLYSIZE_Msk |
                 CRC_CR_REV_IN_Msk |
                 CRC_CR_REV_OUT_Msk);

    CRC->POL  = 0x04C11DB7U;
    CRC->INIT = 0xFFFFFFFFU;

    /* Load INIT into CRC computation unit */
    CRC->CR |= CRC_CR_RESET;
}

uint32_t crc32_calculate(const uint32_t *data, uint32_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return 0U;
    }

    CRC->CR |= CRC_CR_RESET;

    for (uint32_t i = 0U; i < length; i++)
    {
        CRC->DR = data[i];
    }

    return CRC->DR;
}

uint8_t vrefbuf_init(void)
{
    RCC->APB3ENR |= RCC_APB3ENR_VREFEN;
    (void)RCC->APB3ENR;

    /* Wyłącz na czas konfiguracji */
    VREFBUF->CSR &= ~VREFBUF_CSR_ENVR;

    /* VRS = 001 -> około 2.048 V */
    VREFBUF->CSR &= ~VREFBUF_CSR_VRS_Msk;
    VREFBUF->CSR |= (1U << VREFBUF_CSR_VRS_Pos);

    /*
     * HIZ = 0:
     * wewnętrzny buffer połączony z VREF+
     */
    VREFBUF->CSR &= ~VREFBUF_CSR_HIZ;

    /* Enable */
    VREFBUF->CSR |= VREFBUF_CSR_ENVR;

    uint32_t timeout = 1000000U;

    while ((VREFBUF->CSR & VREFBUF_CSR_VRR) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    return 1U;
}

uint8_t dts_init(void)
{
    RCC->APB1HENR |= RCC_APB1HENR_DTSEN;
    (void)RCC->APB1HENR;

    if ((RCC->BDCR & RCC_BDCR_LSERDY) == 0U)
    {
        return 0U;
    }

    DTS->CFGR1 &=
        ~(DTS_CFGR1_TS1_EN |
          DTS_CFGR1_TS1_START |
          DTS_CFGR1_Q_MEAS_OPT |
          DTS_CFGR1_TS1_INTRIG_SEL_Msk |
          DTS_CFGR1_TS1_SMP_TIME_Msk |
          DTS_CFGR1_HSREF_CLK_DIV_Msk);

    /*
     * Reference clock = LSE.
     */
    DTS->CFGR1 |= DTS_CFGR1_REFCLK_SEL;

    /*
     * Slow measure / calibration enabled.
     * Q_MEAS_OPT = 0
     */

    /*
     * Software trigger.
     * TS1_INTRIG_SEL = 0
     */

    DTS->CFGR1 |=
        (15U << DTS_CFGR1_TS1_SMP_TIME_Pos);

    DTS->CFGR1 |= DTS_CFGR1_TS1_EN;

    uint32_t timeout = 1000000U;

    while ((DTS->SR & DTS_SR_TS1_RDY) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    DTS->ICIFR =
        DTS_ICIFR_TS1_CITEF |
        DTS_ICIFR_TS1_CAITEF;

    DTS->CFGR1 |= DTS_CFGR1_TS1_START;

    delay_10us(500U);

    return 1U;
}


uint8_t dts_get_temperature(int32_t *temperature)
{
    if (temperature == NULL)
    {
        return 0U;
    }

    uint32_t samples =
        DTS->DR & DTS_DR_TS1_MFREQ_Msk;

    if (samples == 0U)
    {
        return 0U;
    }

    /* LSE reference, sampling time = 15 cycles */
    uint32_t freq_meas =
        (32768U * samples) / 15U;

    uint32_t t0_code =
        (DTS->T0VALR1 & DTS_T0VALR1_TS1_T0_Msk) >>
        DTS_T0VALR1_TS1_T0_Pos;

    int32_t t0;

    if (t0_code == 0U)
    {
        t0 = 30;
    }
    else if (t0_code == 1U)
    {
        t0 = 130;
    }
    else
    {
        return 0U;
    }

    uint32_t t0_freq =
        (DTS->T0VALR1 & DTS_T0VALR1_TS1_FMT0_Msk) * 100U;

    uint32_t ramp =
        DTS->RAMPVALR &
        DTS_RAMPVALR_TS1_RAMP_COEFF_Msk;

    if (ramp == 0U)
    {
        return 0U;
    }

    *temperature =
        t0 +
        (((int32_t)freq_meas -
          (int32_t)t0_freq) /
         (int32_t)ramp);

    return 1U;
}

void cordic_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CORDICEN;
    (void)RCC->AHB1ENR;

    CORDIC->CSR =
        (0U << CORDIC_CSR_FUNC_Pos)      |  /* cosine */
        (6U << CORDIC_CSR_PRECISION_Pos) |  /* 6 cycles */
        CORDIC_CSR_NRES;                    /* 2 results */

    /*
     * SCALE   = 0
     * NARGS   = 0 -> 1 write
     * NRES    = 1 -> 2 reads
     * ARGSIZE = 0 -> 32 bit
     * RESSIZE = 0 -> 32 bit
     */
}

uint8_t cordic_cos_sin(int32_t angle,
                       int32_t *cos_result,
                       int32_t *sin_result)
{
    if ((cos_result == NULL) || (sin_result == NULL))
    {
        return 0U;
    }

    CORDIC->WDATA = (uint32_t)angle;

    uint32_t timeout = 100000U;

    while ((CORDIC->CSR & CORDIC_CSR_RRDY) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    *cos_result = (int32_t)CORDIC->RDATA;
    *sin_result = (int32_t)CORDIC->RDATA;

    return 1U;
}

void pwr_enter_stop(void)
{
    /*
     * LPMS = 0 -> STOP
     */
    PWR->PMCR &= ~PWR_PMCR_LPMS;

    /*
     * Deep sleep zamiast zwykłego Sleep.
     */
    SCB->SCR |= SCB_SCR_SLEEPDEEP_Msk;

    /*
     * Upewnij się, że wszystkie operacje pamięci
     * i pipeline są zakończone.
     */
    __DSB();
    __ISB();

    /*
     * CPU wchodzi w STOP.
     */
    __WFI();

    /*
     * Tutaj jesteśmy dopiero po wake-up.
     */
    SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
}