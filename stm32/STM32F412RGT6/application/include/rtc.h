#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "stm32f4xx.h"

void rtc_init(void);

void rtc_write_protect_disable(void);
void rtc_write_protect_enable(void);

void rtc_exti_clear(uint32_t line);

void rtc_write_time(uint8_t hours, uint8_t minutes, uint8_t seconds);
void rtc_write_date(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday);

void rtc_read_time(uint8_t *hours, uint8_t *minutes, uint8_t *seconds);
void rtc_read_date(uint8_t *year, uint8_t *month, uint8_t *date, uint8_t *weekday);

void rtc_read_datetime(uint8_t *year,uint8_t *month,uint8_t *day,uint8_t *weekday,
                    uint8_t *hours,uint8_t *minutes,uint8_t *seconds);
int rtc_set_datetime(uint8_t year, uint8_t month, uint8_t date, uint8_t weekday,
                    uint8_t hours, uint8_t minutes, uint8_t seconds);

void rtc_wakeup_disable(void);
int  rtc_wakeup_start_seconds(uint16_t seconds);

void rtc_alarmA_disable(void);
int  rtc_alarmA_set_hms(uint8_t h, uint8_t m, uint8_t s, uint8_t daily);
int  rtc_alarmA_set_day_hms(uint8_t day, uint8_t hours, uint8_t minutes, uint8_t seconds);

void rtc_timestamp_enable_rising(void);
int  rtc_timestamp_read(uint8_t *mo, uint8_t *dd, uint8_t *wd,
                        uint8_t *hh, uint8_t *min, uint8_t *ss);

void    rtc_tamper1_enable(uint8_t rising_edge);
uint8_t rtc_tamper1_get_and_clear(void);

#endif // RTC_H