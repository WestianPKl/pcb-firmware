#include "rtc.h"

#define RTC_INIT_MARKER  0x32F2U

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10U + (bcd & 0x0FU);
}

static uint8_t bin_to_bcd(uint8_t bin) {
    return (uint8_t)(((bin / 10U) << 4) | (bin % 10U));
}

void rtc_init(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;

    PWR->CR1 |= PWR_CR1_DBP;

    if (RTC->BKP0R == RTC_INIT_MARKER) {
        return;
    }

    RCC->BDCR |= RCC_BDCR_LSEON;
    uint32_t t = 5000000U;
    while (!(RCC->BDCR & RCC_BDCR_LSERDY) && --t);

    RCC->BDCR &= ~RCC_BDCR_RTCSEL;
    if (t != 0U) {
        RCC->BDCR |= RCC_BDCR_RTCSEL_0;
    } else {
        RCC->CSR |= RCC_CSR_LSION;
        t = 5000000U;
        while (!(RCC->CSR & RCC_CSR_LSIRDY) && --t);
        RCC->BDCR |= RCC_BDCR_RTCSEL_1;
    }
    RCC->BDCR |= RCC_BDCR_RTCEN;

    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    RTC->ISR |= RTC_ISR_INIT;
    t = 1000000U;
    while (!(RTC->ISR & RTC_ISR_INITF) && --t);
    if (t == 0U) {
        RTC->WPR = 0xFFU;
        return;
    }

    RTC->PRER  = (0xFFU << RTC_PRER_PREDIV_S_Pos);
    RTC->PRER |= (0x7FU << RTC_PRER_PREDIV_A_Pos);


    RTC->TR = (bin_to_bcd(0) << RTC_TR_HU_Pos)
            | (bin_to_bcd(0) << RTC_TR_MNU_Pos)
            | (bin_to_bcd(0) << RTC_TR_SU_Pos);
    RTC->DR = (bin_to_bcd(11) << RTC_DR_YU_Pos)
            | (bin_to_bcd(1)  << RTC_DR_MU_Pos)
            | (bin_to_bcd(1)  << RTC_DR_DU_Pos)
            | (bin_to_bcd(6)  << RTC_DR_WDU_Pos);

    RTC->ISR &= ~RTC_ISR_INIT;
    RTC->WPR = 0xFFU;

    RTC->BKP0R = RTC_INIT_MARKER;
}

void rtc_get_datetime(rtc_datetime_t *datetime)
{
    PWR->CR1 |= PWR_CR1_DBP;

    RTC->ISR &= ~RTC_ISR_RSF;
    uint32_t t = 1000000U;
    while (!(RTC->ISR & RTC_ISR_RSF) && --t);
    if (t == 0U) { return; }

    uint32_t tr = RTC->TR;
    uint32_t dr = RTC->DR;

    datetime->seconds = bcd_to_bin((tr & (RTC_TR_ST | RTC_TR_SU))   >> RTC_TR_SU_Pos);
    datetime->minutes = bcd_to_bin((tr & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos);
    datetime->hours   = bcd_to_bin((tr & (RTC_TR_HT | RTC_TR_HU))   >> RTC_TR_HU_Pos);
    datetime->day     = bcd_to_bin((dr & (RTC_DR_DT | RTC_DR_DU))   >> RTC_DR_DU_Pos);
    datetime->month   = bcd_to_bin((dr & (RTC_DR_MT | RTC_DR_MU))   >> RTC_DR_MU_Pos);
    datetime->year    = bcd_to_bin((dr & (RTC_DR_YT | RTC_DR_YU))   >> RTC_DR_YU_Pos) + 2000U;
    datetime->weekday = bcd_to_bin((dr & RTC_DR_WDU)               >> RTC_DR_WDU_Pos);
}

void rtc_set_datetime(const rtc_datetime_t *datetime)
{
    PWR->CR1 |= PWR_CR1_DBP;

    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    RTC->ISR |= RTC_ISR_INIT;
    uint32_t t = 1000000U;
    while (!(RTC->ISR & RTC_ISR_INITF) && --t);
    if (t == 0U) {
        RTC->WPR = 0xFFU;
        return;
    }

    RTC->TR = (bin_to_bcd(datetime->hours)   << RTC_TR_HU_Pos)
            | (bin_to_bcd(datetime->minutes) << RTC_TR_MNU_Pos)
            | (bin_to_bcd(datetime->seconds) << RTC_TR_SU_Pos);
    RTC->DR = (bin_to_bcd((uint8_t)(datetime->year - 2000U)) << RTC_DR_YU_Pos)
            | (bin_to_bcd(datetime->month)   << RTC_DR_MU_Pos)
            | (bin_to_bcd(datetime->day)     << RTC_DR_DU_Pos)
            | (bin_to_bcd(datetime->weekday) << RTC_DR_WDU_Pos);

    RTC->ISR &= ~RTC_ISR_INIT;
    RTC->WPR = 0xFFU;

    RTC->BKP0R = RTC_INIT_MARKER;
}