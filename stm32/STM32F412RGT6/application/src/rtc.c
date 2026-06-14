#include "rtc.h"
#include "systick.h"

#define RTC_WPR_KEY1 0xCAU
#define RTC_WPR_KEY2 0x53U

#define RTC_PREDIV_A 127U
#define RTC_PREDIV_S 255U

#define RTC_BKP_MAGIC 0x32F2A4B1U

#define RTC_TIMEOUT_LOOP   500000U
#define LSE_STARTUP_MS     3000U
#define LSI_STARTUP_MS     200U

volatile uint8_t rtc_busy = 0;

static int wait_mask_set_loop(volatile uint32_t *reg, uint32_t mask, uint32_t timeout)
{
    uint32_t t = timeout;
    while (((*reg) & mask) == 0U) {
        if (--t == 0U) return -1;
    }
    return 0;
}

static int wait_flag_ms(volatile uint32_t *reg, uint32_t mask, uint32_t timeout_ms)
{
    while (timeout_ms--) {
        if (((*reg) & mask) != 0U) return 0;
        systick_delay_ms(1);
    }
    return -1;
}

static uint8_t to_bcd(uint8_t v)
{
    return (uint8_t)(((v / 10U) << 4) | (v % 10U));
}

static inline uint32_t bcd_units(uint8_t bcd) { return (uint32_t)(bcd & 0x0FU); }
static inline uint32_t bcd_tens(uint8_t bcd)  { return (uint32_t)((bcd >> 4) & 0x0FU); }

static void rtc_backup_domain_unlock(void)
{
    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;

    PWR->CR |= PWR_CR_DBP;
    uint32_t t = RTC_TIMEOUT_LOOP;
    while ((PWR->CR & PWR_CR_DBP) == 0U) {
        if (--t == 0U) break;
    }
}

static void rtc_backup_domain_reset(void)
{
    RCC->BDCR |= RCC_BDCR_BDRST;
    __DSB(); __ISB();
    RCC->BDCR &= ~RCC_BDCR_BDRST;
    __DSB(); __ISB();
}

void rtc_write_protect_disable(void)
{
    RTC->WPR = RTC_WPR_KEY1;
    RTC->WPR = RTC_WPR_KEY2;
}

void rtc_write_protect_enable(void)
{
    RTC->WPR = 0xFFU;
}

static int rtc_enter_init_mode(void)
{
    RTC->ISR |= RTC_ISR_INIT;
    return wait_mask_set_loop(&RTC->ISR, RTC_ISR_INITF, RTC_TIMEOUT_LOOP);
}

static void rtc_exit_init_mode(void)
{
    RTC->ISR &= ~RTC_ISR_INIT;
}

static int rtc_wait_for_synchro(void)
{
    rtc_write_protect_disable();
    RTC->ISR &= ~RTC_ISR_RSF;
    rtc_write_protect_enable();

    return wait_mask_set_loop(&RTC->ISR, RTC_ISR_RSF, RTC_TIMEOUT_LOOP);
}

static void rtc_exti_enable_rising(uint32_t line)
{
    EXTI->PR   = (1UL << line);
    EXTI->IMR  |= (1UL << line);
    EXTI->RTSR |= (1UL << line);
    EXTI->FTSR &= ~(1UL << line);
}

static void rtc_exti_enable_falling(uint32_t line)
{
    EXTI->PR   = (1UL << line);
    EXTI->IMR  |= (1UL << line);
    EXTI->FTSR |= (1UL << line);
    EXTI->RTSR &= ~(1UL << line);
}

void rtc_exti_clear(uint32_t line)
{
    EXTI->PR = (1UL << line);
}

static int rtc_select_clock_lse(void)
{
    const uint32_t RTCSEL_LSE = (1U << RCC_BDCR_RTCSEL_Pos);

    uint32_t rtcsel = (RCC->BDCR & RCC_BDCR_RTCSEL_Msk);
    uint32_t rtcen  = (RCC->BDCR & RCC_BDCR_RTCEN);

    if (rtcen && (rtcsel != 0U) && (rtcsel != RTCSEL_LSE)) {
        rtc_backup_domain_reset();
    }

    RCC->BDCR |= RCC_BDCR_LSEON;

    if (wait_flag_ms(&RCC->BDCR, RCC_BDCR_LSERDY, LSE_STARTUP_MS) < 0) {
        return -1;
    }

    RCC->BDCR &= ~RCC_BDCR_RTCSEL_Msk;
    RCC->BDCR |=  RTCSEL_LSE;
    RCC->BDCR |=  RCC_BDCR_RTCEN;
    return 0;
}

static int rtc_select_clock_lsi(void)
{
    const uint32_t RTCSEL_LSI = (2U << RCC_BDCR_RTCSEL_Pos);

    uint32_t rtcsel = (RCC->BDCR & RCC_BDCR_RTCSEL_Msk);
    uint32_t rtcen  = (RCC->BDCR & RCC_BDCR_RTCEN);

    if (rtcen && (rtcsel != 0U) && (rtcsel != RTCSEL_LSI)) {
        rtc_backup_domain_reset();
    }

    RCC->CSR |= RCC_CSR_LSION;

    if (wait_flag_ms(&RCC->CSR, RCC_CSR_LSIRDY, LSI_STARTUP_MS) < 0) {
        if (wait_mask_set_loop(&RCC->CSR, RCC_CSR_LSIRDY, RTC_TIMEOUT_LOOP) < 0) {
            return -1;
        }
    }

    RCC->BDCR &= ~RCC_BDCR_RTCSEL_Msk;
    RCC->BDCR |=  RTCSEL_LSI;
    RCC->BDCR |=  RCC_BDCR_RTCEN;
    return 0;
}

static uint8_t rtc_is_configured(void)
{
    if ((RCC->BDCR & RCC_BDCR_RTCEN) == 0U) return 0U;
    return (RTC->BKP0R == RTC_BKP_MAGIC) ? 1U : 0U;
}

void rtc_init(void)
{
    rtc_backup_domain_unlock();

    if (rtc_is_configured()) {

        uint32_t rtcsel = (RCC->BDCR & RCC_BDCR_RTCSEL_Msk) >> RCC_BDCR_RTCSEL_Pos;

        if (rtcsel == 2U) {
            RCC->BDCR |= RCC_BDCR_LSEON;
            if (wait_flag_ms(&RCC->BDCR, RCC_BDCR_LSERDY, LSE_STARTUP_MS) == 0) {
                rtc_backup_domain_reset();
            } else {
                RCC->CSR |= RCC_CSR_LSION;
                (void)wait_flag_ms(&RCC->CSR, RCC_CSR_LSIRDY, LSI_STARTUP_MS);
                (void)rtc_wait_for_synchro();
                return;
            }
        } else {
            (void)rtc_wait_for_synchro();
            return;
        }
    }

    if (rtc_select_clock_lse() < 0) {
        (void)rtc_select_clock_lsi();
    }

    rtc_write_protect_disable();
    if (rtc_enter_init_mode() < 0) {
        rtc_write_protect_enable();
        return;
    }

    RTC->CR = 0;

    RTC->PRER =
        ((RTC_PREDIV_A & 0x7FU)   << RTC_PRER_PREDIV_A_Pos) |
        ((RTC_PREDIV_S & 0x7FFFU) << RTC_PRER_PREDIV_S_Pos);

    rtc_exit_init_mode();

    RTC->BKP0R = RTC_BKP_MAGIC;

    rtc_write_protect_enable();

    (void)rtc_wait_for_synchro();
}

void rtc_write_time(uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (hours > 23U) hours = 23U;
    if (minutes > 59U) minutes = 59U;
    if (seconds > 59U) seconds = 59U;

    uint8_t hb = to_bcd(hours);
    uint8_t mb = to_bcd(minutes);
    uint8_t sb = to_bcd(seconds);

    rtc_write_protect_disable();
    if (rtc_enter_init_mode() < 0) {
        rtc_write_protect_enable();
        return;
    }

    RTC->TR =
        (bcd_tens(hb)  << RTC_TR_HT_Pos)  | (bcd_units(hb) << RTC_TR_HU_Pos)  |
        (bcd_tens(mb)  << RTC_TR_MNT_Pos) | (bcd_units(mb) << RTC_TR_MNU_Pos) |
        (bcd_tens(sb)  << RTC_TR_ST_Pos)  | (bcd_units(sb) << RTC_TR_SU_Pos);

    rtc_exit_init_mode();
    rtc_write_protect_enable();

    (void)rtc_wait_for_synchro();
}

void rtc_write_date(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday)
{
    if (year > 99U) year = (uint8_t)(year % 100U);
    if (month < 1U) month = 1U; else if (month > 12U) month = 12U;
    if (date  < 1U) date  = 1U; else if (date  > 31U) date  = 31U;
    if (weekday < 1U) weekday = 1U; else if (weekday > 7U) weekday = 7U;

    uint8_t yb = to_bcd(year);
    uint8_t mb = to_bcd(month);
    uint8_t db = to_bcd(date);

    rtc_write_protect_disable();
    if (rtc_enter_init_mode() < 0) {
        rtc_write_protect_enable();
        return;
    }

    RTC->DR =
        (bcd_tens(yb) << RTC_DR_YT_Pos) | (bcd_units(yb) << RTC_DR_YU_Pos) |
        (bcd_tens(mb) << RTC_DR_MT_Pos) | (bcd_units(mb) << RTC_DR_MU_Pos) |
        (bcd_tens(db) << RTC_DR_DT_Pos) | (bcd_units(db) << RTC_DR_DU_Pos) |
        ((uint32_t)weekday << RTC_DR_WDU_Pos);

    rtc_exit_init_mode();
    rtc_write_protect_enable();

    (void)rtc_wait_for_synchro();
}

int rtc_set_datetime(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday,
                     uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    rtc_busy = 1;

    rtc_backup_domain_unlock();

    if (year > 99U ||
        month < 1U || month > 12U ||
        date  < 1U || date  > 31U ||
        weekday < 1U || weekday > 7U ||
        hours > 23U || minutes > 59U || seconds > 59U) {
        rtc_busy = 0;
        return -3;
    }

    uint8_t yb  = to_bcd((uint8_t)(year % 100U));
    uint8_t mb  = to_bcd(month);
    uint8_t db  = to_bcd(date);
    uint8_t hb  = to_bcd(hours);
    uint8_t mib = to_bcd(minutes);
    uint8_t sb  = to_bcd(seconds);

    rtc_write_protect_disable();

    if (rtc_enter_init_mode() < 0) {
        rtc_write_protect_enable();
        rtc_busy = 0;
        return -1;
    }

    RTC->DR =
        (bcd_tens(yb) << RTC_DR_YT_Pos) | (bcd_units(yb) << RTC_DR_YU_Pos) |
        (bcd_tens(mb) << RTC_DR_MT_Pos) | (bcd_units(mb) << RTC_DR_MU_Pos) |
        (bcd_tens(db) << RTC_DR_DT_Pos) | (bcd_units(db) << RTC_DR_DU_Pos) |
        ((uint32_t)weekday << RTC_DR_WDU_Pos);

    RTC->TR =
        (bcd_tens(hb)  << RTC_TR_HT_Pos)  | (bcd_units(hb) << RTC_TR_HU_Pos)  |
        (bcd_tens(mib) << RTC_TR_MNT_Pos) | (bcd_units(mib) << RTC_TR_MNU_Pos) |
        (bcd_tens(sb)  << RTC_TR_ST_Pos)  | (bcd_units(sb) << RTC_TR_SU_Pos);

    rtc_exit_init_mode();
    rtc_write_protect_enable();

    (void)rtc_wait_for_synchro();

    rtc_busy = 0;
    return 0;
}

void rtc_read_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    if (!hours || !minutes || !seconds) return;

    uint32_t tr1, tr2;
    do { tr1 = RTC->TR; tr2 = RTC->TR; } while (tr1 != tr2);

    uint8_t ht = (uint8_t)((tr1 & RTC_TR_HT_Msk)  >> RTC_TR_HT_Pos);
    uint8_t hu = (uint8_t)((tr1 & RTC_TR_HU_Msk)  >> RTC_TR_HU_Pos);
    uint8_t mt = (uint8_t)((tr1 & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos);
    uint8_t mu = (uint8_t)((tr1 & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos);
    uint8_t st = (uint8_t)((tr1 & RTC_TR_ST_Msk)  >> RTC_TR_ST_Pos);
    uint8_t su = (uint8_t)((tr1 & RTC_TR_SU_Msk)  >> RTC_TR_SU_Pos);

    *hours   = (uint8_t)(ht * 10U + hu);
    *minutes = (uint8_t)(mt * 10U + mu);
    *seconds = (uint8_t)(st * 10U + su);
}

void rtc_read_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *weekday)
{
    if (!year || !month || !date || !weekday) return;

    uint32_t dr1, dr2;
    do { dr1 = RTC->DR; dr2 = RTC->DR; } while (dr1 != dr2);

    uint8_t yt = (uint8_t)((dr1 & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos);
    uint8_t yu = (uint8_t)((dr1 & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos);
    uint8_t mt = (uint8_t)((dr1 & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos);
    uint8_t mu = (uint8_t)((dr1 & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos);
    uint8_t dt = (uint8_t)((dr1 & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos);
    uint8_t du = (uint8_t)((dr1 & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos);

    *year    = (uint8_t)(yt * 10U + yu);
    *month   = (uint8_t)(mt * 10U + mu);
    *date    = (uint8_t)(dt * 10U + du);
    *weekday = (uint8_t)((dr1 & RTC_DR_WDU_Msk) >> RTC_DR_WDU_Pos);
}

void rtc_read_datetime(uint8_t *year, uint8_t *month, uint8_t *day, uint8_t *weekday,
                       uint8_t *hours, uint8_t *minutes, uint8_t *seconds)
{
    if (!year || !month || !day || !weekday || !hours || !minutes || !seconds) return;

    uint32_t tr1, tr2, dr1, dr2;
    do {
        tr1 = RTC->TR;
        dr1 = RTC->DR;
        tr2 = RTC->TR;
        dr2 = RTC->DR;
    } while (tr1 != tr2 || dr1 != dr2);

    uint8_t ht = (uint8_t)((tr1 & RTC_TR_HT_Msk)  >> RTC_TR_HT_Pos);
    uint8_t hu = (uint8_t)((tr1 & RTC_TR_HU_Msk)  >> RTC_TR_HU_Pos);
    uint8_t mt = (uint8_t)((tr1 & RTC_TR_MNT_Msk) >> RTC_TR_MNT_Pos);
    uint8_t mu = (uint8_t)((tr1 & RTC_TR_MNU_Msk) >> RTC_TR_MNU_Pos);
    uint8_t st = (uint8_t)((tr1 & RTC_TR_ST_Msk)  >> RTC_TR_ST_Pos);
    uint8_t su = (uint8_t)((tr1 & RTC_TR_SU_Msk)  >> RTC_TR_SU_Pos);

    *hours   = (uint8_t)(ht * 10U + hu);
    *minutes = (uint8_t)(mt * 10U + mu);
    *seconds = (uint8_t)(st * 10U + su);

    uint8_t yt = (uint8_t)((dr1 & RTC_DR_YT_Msk) >> RTC_DR_YT_Pos);
    uint8_t yu = (uint8_t)((dr1 & RTC_DR_YU_Msk) >> RTC_DR_YU_Pos);
    uint8_t mot = (uint8_t)((dr1 & RTC_DR_MT_Msk) >> RTC_DR_MT_Pos);
    uint8_t mou = (uint8_t)((dr1 & RTC_DR_MU_Msk) >> RTC_DR_MU_Pos);
    uint8_t dt = (uint8_t)((dr1 & RTC_DR_DT_Msk) >> RTC_DR_DT_Pos);
    uint8_t du = (uint8_t)((dr1 & RTC_DR_DU_Msk) >> RTC_DR_DU_Pos);

    *year    = (uint8_t)(yt * 10U + yu);
    *month   = (uint8_t)(mot * 10U + mou);
    *day     = (uint8_t)(dt * 10U + du);
    *weekday = (uint8_t)((dr1 & RTC_DR_WDU_Msk) >> RTC_DR_WDU_Pos);
}

void rtc_alarmA_disable(void)
{
    rtc_write_protect_disable();

    RTC->CR &= ~RTC_CR_ALRAIE;

    RTC->CR &= ~RTC_CR_ALRAE;
    (void)wait_mask_set_loop(&RTC->ISR, RTC_ISR_ALRAWF, RTC_TIMEOUT_LOOP);

    RTC->ISR &= ~RTC_ISR_ALRAF;

    rtc_write_protect_enable();
    rtc_exti_clear(18U);
}

int rtc_alarmA_set_hms(uint8_t h, uint8_t m, uint8_t s, uint8_t daily)
{
    if (h > 23U || m > 59U || s > 59U) return -1;

    uint8_t hb = to_bcd(h);
    uint8_t mb = to_bcd(m);
    uint8_t sb = to_bcd(s);

    rtc_write_protect_disable();

    RTC->CR &= ~RTC_CR_ALRAE;
    if (wait_mask_set_loop(&RTC->ISR, RTC_ISR_ALRAWF, RTC_TIMEOUT_LOOP) < 0) {
        rtc_write_protect_enable();
        return -2;
    }

    uint32_t alrmar = 0;
    alrmar |= (bcd_units(sb) << RTC_ALRMAR_SU_Pos)  | (bcd_tens(sb) << RTC_ALRMAR_ST_Pos);
    alrmar |= (bcd_units(mb) << RTC_ALRMAR_MNU_Pos) | (bcd_tens(mb) << RTC_ALRMAR_MNT_Pos);
    alrmar |= (bcd_units(hb) << RTC_ALRMAR_HU_Pos)  | (bcd_tens(hb) << RTC_ALRMAR_HT_Pos);

    if (daily) {
        alrmar |= RTC_ALRMAR_MSK4;
    } else {
        uint8_t yy, mo, dd, wd;
        rtc_read_date(&yy, &mo, &dd, &wd);
        uint8_t db = to_bcd(dd);
        alrmar |= (bcd_units(db) << RTC_ALRMAR_DU_Pos) | (bcd_tens(db) << RTC_ALRMAR_DT_Pos);
        alrmar &= ~RTC_ALRMAR_WDSEL;
        alrmar &= ~RTC_ALRMAR_MSK4;
    }

    RTC->ALRMAR = alrmar;

    RTC->ISR &= ~RTC_ISR_ALRAF;
    RTC->CR  |= RTC_CR_ALRAIE;
    RTC->CR  |= RTC_CR_ALRAE;

    rtc_write_protect_enable();

    rtc_exti_enable_rising(18U);
    NVIC_EnableIRQ(RTC_Alarm_IRQn);
    return 0;
}

int rtc_alarmA_set_day_hms(uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds)
{
    if (day < 1U || day > 31U) return -1;
    if (hours > 23U || minutes > 59U || seconds > 59U) return -1;

    uint8_t hb = to_bcd(hours);
    uint8_t mb = to_bcd(minutes);
    uint8_t sb = to_bcd(seconds);
    uint8_t db = to_bcd(day);

    rtc_write_protect_disable();

    RTC->CR &= ~RTC_CR_ALRAE;
    if (wait_mask_set_loop(&RTC->ISR, RTC_ISR_ALRAWF, RTC_TIMEOUT_LOOP) < 0) {
        rtc_write_protect_enable();
        return -2;
    }

    uint32_t alrmar = 0;
    alrmar |= (bcd_units(sb) << RTC_ALRMAR_SU_Pos)  | (bcd_tens(sb) << RTC_ALRMAR_ST_Pos);
    alrmar |= (bcd_units(mb) << RTC_ALRMAR_MNU_Pos) | (bcd_tens(mb) << RTC_ALRMAR_MNT_Pos);
    alrmar |= (bcd_units(hb) << RTC_ALRMAR_HU_Pos)  | (bcd_tens(hb) << RTC_ALRMAR_HT_Pos);
    alrmar |= (bcd_units(db) << RTC_ALRMAR_DU_Pos)  | (bcd_tens(db) << RTC_ALRMAR_DT_Pos);

    alrmar &= ~RTC_ALRMAR_WDSEL;
    alrmar &= ~RTC_ALRMAR_MSK4;

    RTC->ALRMAR = alrmar;

    RTC->ISR &= ~RTC_ISR_ALRAF;
    RTC->CR  |= RTC_CR_ALRAIE;
    RTC->CR  |= RTC_CR_ALRAE;

    rtc_write_protect_enable();

    rtc_exti_enable_rising(18U);
    NVIC_EnableIRQ(RTC_Alarm_IRQn);
    return 0;
}

void rtc_timestamp_enable_rising(void)
{
    rtc_write_protect_disable();

    RTC->CR &= ~RTC_CR_TSEDGE;
    RTC->ISR &= ~(RTC_ISR_TSF | RTC_ISR_TSOVF);
    RTC->CR |= RTC_CR_TSIE;
    RTC->CR |= RTC_CR_TSE;

    rtc_write_protect_enable();

    rtc_exti_clear(19U);
    rtc_exti_enable_rising(19U);
    NVIC_EnableIRQ(TAMP_STAMP_IRQn);
}

int rtc_timestamp_read(uint8_t *mo, uint8_t *dd, uint8_t *wd,
                       uint8_t *hh, uint8_t *min, uint8_t *ss)
{
    if (!mo || !dd || !wd || !hh || !min || !ss) return -1;
    if ((RTC->ISR & RTC_ISR_TSF) == 0U) return -2;

    uint32_t tstr = RTC->TSTR;
    uint32_t tsdr = RTC->TSDR;

    uint8_t ht = (uint8_t)((tstr & RTC_TSTR_HT_Msk)  >> RTC_TSTR_HT_Pos);
    uint8_t hu = (uint8_t)((tstr & RTC_TSTR_HU_Msk)  >> RTC_TSTR_HU_Pos);
    uint8_t mt = (uint8_t)((tstr & RTC_TSTR_MNT_Msk) >> RTC_TSTR_MNT_Pos);
    uint8_t mu = (uint8_t)((tstr & RTC_TSTR_MNU_Msk) >> RTC_TSTR_MNU_Pos);
    uint8_t st = (uint8_t)((tstr & RTC_TSTR_ST_Msk)  >> RTC_TSTR_ST_Pos);
    uint8_t su = (uint8_t)((tstr & RTC_TSTR_SU_Msk)  >> RTC_TSTR_SU_Pos);

    *hh  = (uint8_t)(ht * 10U + hu);
    *min = (uint8_t)(mt * 10U + mu);
    *ss  = (uint8_t)(st * 10U + su);

    uint8_t mtens  = (uint8_t)((tsdr & RTC_TSDR_MT_Msk) >> RTC_TSDR_MT_Pos);
    uint8_t munits = (uint8_t)((tsdr & RTC_TSDR_MU_Msk) >> RTC_TSDR_MU_Pos);
    uint8_t dtens  = (uint8_t)((tsdr & RTC_TSDR_DT_Msk) >> RTC_TSDR_DT_Pos);
    uint8_t dunits = (uint8_t)((tsdr & RTC_TSDR_DU_Msk) >> RTC_TSDR_DU_Pos);

    *mo = (uint8_t)(mtens * 10U + munits);
    *dd = (uint8_t)(dtens * 10U + dunits);
    *wd = (uint8_t)((tsdr & RTC_TSDR_WDU_Msk) >> RTC_TSDR_WDU_Pos);

    rtc_write_protect_disable();
    RTC->ISR &= ~(RTC_ISR_TSF | RTC_ISR_TSOVF);
    rtc_write_protect_enable();

    return 0;
}

void rtc_tamper1_enable(uint8_t rising_edge)
{
    rtc_backup_domain_unlock();

    rtc_write_protect_disable();

    RTC->ISR &= ~RTC_ISR_TAMP1F;

    if (rising_edge) RTC->TAFCR &= ~RTC_TAFCR_TAMP1TRG;
    else            RTC->TAFCR |=  RTC_TAFCR_TAMP1TRG;

    RTC->TAFCR |= RTC_TAFCR_TAMPIE;
    RTC->TAFCR |= RTC_TAFCR_TAMP1E;

    rtc_write_protect_enable();

    rtc_exti_clear(19U);
    if (rising_edge) rtc_exti_enable_rising(19U);
    else            rtc_exti_enable_falling(19U);

    NVIC_ClearPendingIRQ(TAMP_STAMP_IRQn);
    NVIC_EnableIRQ(TAMP_STAMP_IRQn);
}

uint8_t rtc_tamper1_get_and_clear(void)
{
    uint8_t f = (RTC->ISR & RTC_ISR_TAMP1F) ? 1U : 0U;
    if (f) {
        rtc_write_protect_disable();
        RTC->ISR &= ~RTC_ISR_TAMP1F;
        rtc_write_protect_enable();
    }
    return f;
}

void rtc_wakeup_disable(void)
{
    rtc_write_protect_disable();

    RTC->CR &= ~RTC_CR_WUTIE;

    RTC->CR &= ~RTC_CR_WUTE;
    (void)wait_mask_set_loop(&RTC->ISR, RTC_ISR_WUTWF, RTC_TIMEOUT_LOOP);

    RTC->ISR &= ~RTC_ISR_WUTF;

    rtc_write_protect_enable();
}

int rtc_wakeup_start_seconds(uint16_t seconds)
{
    if (seconds == 0U) return -1;

    rtc_write_protect_disable();

    RTC->CR &= ~RTC_CR_WUTE;
    if (wait_mask_set_loop(&RTC->ISR, RTC_ISR_WUTWF, RTC_TIMEOUT_LOOP) < 0) {
        rtc_write_protect_enable();
        return -2;
    }

    RTC->CR &= ~RTC_CR_WUCKSEL_Msk;
    RTC->CR |=  (4U << RTC_CR_WUCKSEL_Pos);

    RTC->WUTR = (uint32_t)(seconds - 1U);

    RTC->ISR &= ~RTC_ISR_WUTF;
    RTC->CR  |= RTC_CR_WUTIE;
    RTC->CR  |= RTC_CR_WUTE;

    rtc_write_protect_enable();

    rtc_exti_enable_rising(20U);
    NVIC_EnableIRQ(RTC_WKUP_IRQn);
    return 0;
}