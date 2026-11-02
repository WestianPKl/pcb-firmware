#include "rtc.h"

#define RTC_BACKUP_MAGIC 0xA55AA55AU

volatile uint8_t rtc_alarm_flag = 0U;
volatile uint8_t rtc_wakeup_flag = 0U;
volatile uint8_t tamp1_detected = 0U;

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10U + (bcd & 0x0FU);
}

static uint8_t bin_to_bcd(uint8_t bin) {
    return (uint8_t)(((bin / 10U) << 4) | (bin % 10U));
}

uint8_t rtc_clock_init(void)
{
    RCC->APB3ENR |= RCC_APB3ENR_RTCAPBEN;
    (void)RCC->APB3ENR;

    PWR->DBPCR |= PWR_DBPCR_DBP;

    uint32_t timeout = 1000000U;

    while ((PWR->DBPCR & PWR_DBPCR_DBP) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    RCC->BDCR |= RCC_BDCR_LSEON;

    timeout = 1000000U;

    while ((RCC->BDCR & RCC_BDCR_LSERDY) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    RCC->BDCR &= ~RCC_BDCR_RTCSEL_Msk;
    RCC->BDCR |= (1U << RCC_BDCR_RTCSEL_Pos);

    RCC->BDCR |= RCC_BDCR_RTCEN;

    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    RTC->ICSR |= RTC_ICSR_INIT;

    timeout = 1000000U;

    while ((RTC->ICSR & RTC_ICSR_INITF) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    RTC->CR &= ~RTC_CR_FMT;

    RTC->PRER = (127U << RTC_PRER_PREDIV_A_Pos) | (255U << RTC_PRER_PREDIV_S_Pos);

    RTC->TR = (1U << RTC_TR_HT_Pos)  | (5U << RTC_TR_HU_Pos)  | (3U << RTC_TR_MNT_Pos);

    RTC->DR = (1U << RTC_DR_DU_Pos) | (1U << RTC_DR_MU_Pos) | (1U << RTC_DR_WDU_Pos);

    RTC->ICSR &= ~RTC_ICSR_INIT;

    RTC->WPR = 0xFFU;

    return 1U;
}

void rtc_get_datetime(rtc_datetime_t *datetime)
{
    if (datetime == NULL)
    {
        return;
    }

    uint32_t tr = RTC->TR;
    uint32_t dr = RTC->DR;

    datetime->seconds = bcd_to_bin((tr & (RTC_TR_ST | RTC_TR_SU)) >> RTC_TR_SU_Pos);
    datetime->minutes = bcd_to_bin((tr & (RTC_TR_MNT | RTC_TR_MNU)) >> RTC_TR_MNU_Pos);
    datetime->hours = bcd_to_bin((tr & (RTC_TR_HT | RTC_TR_HU)) >> RTC_TR_HU_Pos);
    datetime->day = bcd_to_bin((dr & (RTC_DR_DT | RTC_DR_DU)) >> RTC_DR_DU_Pos);
    datetime->month = bcd_to_bin((dr & (RTC_DR_MT | RTC_DR_MU)) >> RTC_DR_MU_Pos);
    datetime->year = bcd_to_bin((dr & (RTC_DR_YT | RTC_DR_YU)) >> RTC_DR_YU_Pos) + 2000U;
    datetime->weekday = (uint8_t)((dr & RTC_DR_WDU) >> RTC_DR_WDU_Pos);
}

uint8_t rtc_set_datetime(const rtc_datetime_t *datetime)
{
    if ((datetime == NULL) || (datetime->year < 2000U) || (datetime->year > 2099U) || (datetime->month < 1U) || (datetime->month > 12U) || (datetime->day < 1U) || (datetime->day > 31U) || (datetime->hours > 23U) || (datetime->minutes > 59U) || (datetime->seconds > 59U) || (datetime->weekday < 1U) || (datetime->weekday > 7U))
    {
        return 0U;
    }

    PWR->DBPCR |= PWR_DBPCR_DBP;

    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    RTC->ICSR |= RTC_ICSR_INIT;

    uint32_t timeout = 1000000U;

    while ((RTC->ICSR & RTC_ICSR_INITF) == 0U)
    {
        if (--timeout == 0U)
        {
            RTC->WPR = 0xFFU;
            return 0U;
        }
    }

    RTC->TR = ((uint32_t)bin_to_bcd(datetime->hours)   << RTC_TR_HU_Pos)  | ((uint32_t)bin_to_bcd(datetime->minutes) << RTC_TR_MNU_Pos) | ((uint32_t)bin_to_bcd(datetime->seconds) << RTC_TR_SU_Pos);

    RTC->DR = ((uint32_t)bin_to_bcd((uint8_t)(datetime->year - 2000U)) << RTC_DR_YU_Pos) | ((uint32_t)bin_to_bcd(datetime->month) << RTC_DR_MU_Pos) | ((uint32_t)bin_to_bcd(datetime->day)   << RTC_DR_DU_Pos) | ((uint32_t)datetime->weekday << RTC_DR_WDU_Pos);

    RTC->ICSR &= ~RTC_ICSR_INIT;

    RTC->WPR = 0xFFU;

    return 1U;
}



void rtc_backup_write(uint32_t value)
{
    PWR->DBPCR |= PWR_DBPCR_DBP;

    TAMP->BKP0R = value;
}

uint32_t rtc_backup_read(void)
{
    return TAMP->BKP0R;
}



uint8_t rtc_alarm_init(uint8_t second)
{
    if (second > 59U)
    {
        return 0U;
    }

    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    /* Disable Alarm A */
    RTC->CR &= ~(RTC_CR_ALRAE | RTC_CR_ALRAIE);

    uint32_t timeout = 100000U;

    while ((RTC->ICSR & RTC_ICSR_ALRAWF) == 0U)
    {
        if (--timeout == 0U)
        {
            RTC->WPR = 0xFFU;
            return 0U;
        }
    }

    /* Clear old flag */
    RTC->SCR = RTC_SCR_CALRAF;

    uint32_t sec_bcd = bin_to_bcd(second);

    RTC->ALRMAR =
        (sec_bcd << RTC_ALRMAR_SU_Pos) |
        RTC_ALRMAR_MSK2 |
        RTC_ALRMAR_MSK3 |
        RTC_ALRMAR_MSK4;

    /*
     * MASKSS = 0:
     * no subsecond comparison
     */
    RTC->ALRMASSR = 0U;

    RTC->CR |= RTC_CR_ALRAIE;
    RTC->CR |= RTC_CR_ALRAE;

    RTC->WPR = 0xFFU;

    NVIC_SetPriority(RTC_IRQn, 5U);
    NVIC_EnableIRQ(RTC_IRQn);

    return 1U;
}

uint8_t rtc_wakeup_init(uint16_t seconds)
{
    if (seconds == 0U)
    {
        return 0U;
    }

    RTC->WPR = 0xCAU;
    RTC->WPR = 0x53U;

    /* Disable wake-up timer */
    RTC->CR &= ~(RTC_CR_WUTE | RTC_CR_WUTIE);

    uint32_t timeout = 100000U;

    while ((RTC->ICSR & RTC_ICSR_WUTWF) == 0U)
    {
        if (--timeout == 0U)
        {
            RTC->WPR = 0xFFU;
            return 0U;
        }
    }

    RTC->SCR = RTC_SCR_CWUTF;

    /*
     * ck_spre = 1 Hz
     */
    RTC->CR &= ~RTC_CR_WUCKSEL_Msk;
    RTC->CR |= (4U << RTC_CR_WUCKSEL_Pos);

    /*
     * Period = (WUT + 1) seconds
     */
    RTC->WUTR = (uint32_t)(seconds - 1U);

    RTC->CR |= RTC_CR_WUTIE;
    RTC->CR |= RTC_CR_WUTE;

    RTC->WPR = 0xFFU;

    NVIC_SetPriority(RTC_IRQn, 5U);
    NVIC_EnableIRQ(RTC_IRQn);

    return 1U;
}

void RTC_IRQHandler(void)
{
    if ((RTC->MISR & RTC_MISR_ALRAMF) != 0U)
    {
        RTC->SCR = RTC_SCR_CALRAF;

        rtc_alarm_flag = 1U;
    }

    if ((RTC->MISR & RTC_MISR_WUTMF) != 0U)
    {
        RTC->SCR = RTC_SCR_CWUTF;

        rtc_wakeup_flag = 1U;
    }
}


void tamp_init(void)
{
    /* Dostęp APB do RTC/TAMP */
    RCC->APB3ENR |= RCC_APB3ENR_RTCAPBEN;
    (void)RCC->APB3ENR;

    /* Backup domain write access */
    PWR->DBPCR |= PWR_DBPCR_DBP;

    /*
     * Na czas konfiguracji wyłącz TAMP1.
     */
    TAMP->CR1 &= ~TAMP_CR1_TAMP1E;

    /*
     * Passive tamper:
     *
     * TAMP1TRG = 0 przy TAMPFLT=0
     * => rising edge.
     *
     * TAMP1NOER = 1
     * => nie kasuj backup registers po zdarzeniu.
     */
    TAMP->CR2 &= ~TAMP_CR2_TAMP1TRG;
    TAMP->CR2 |= TAMP_CR2_TAMP1NOERASE;

    /*
     * Bez filtra:
     * reagujemy bezpośrednio na zbocze.
     */
    TAMP->FLTCR &= ~TAMP_FLTCR_TAMPFLT_Msk;

    /* Skasuj starą flagę */
    TAMP->SCR = TAMP_SCR_CTAMP1F;

    /* Interrupt TAMP1 */
    TAMP->IER |= TAMP_IER_TAMP1IE;

    /* Włącz wykrywanie */
    TAMP->CR1 |= TAMP_CR1_TAMP1E;

    NVIC_SetPriority(TAMP_IRQn, 5U);
    NVIC_EnableIRQ(TAMP_IRQn);
}

void TAMP_IRQHandler(void)
{
    if ((TAMP->MISR & TAMP_MISR_TAMP1MF) != 0U)
    {
        TAMP->SCR = TAMP_SCR_CTAMP1F;

        tamp1_detected = 1U;
    }
}