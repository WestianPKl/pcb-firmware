
#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "stm32h5xx.h"

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_datetime_t;

extern volatile uint8_t rtc_alarm_flag;
extern volatile uint8_t rtc_wakeup_flag;

uint8_t rtc_clock_init(void);
void rtc_get_datetime(rtc_datetime_t *datetime);
uint8_t rtc_set_datetime(const rtc_datetime_t *datetime);
void rtc_backup_write(uint32_t value);
uint32_t rtc_backup_read(void);
uint8_t rtc_alarm_init(uint8_t second);
uint8_t rtc_wakeup_init(uint16_t seconds);

#endif // RTC_H