#ifndef RTC_H
#define RTC_H

#include <stdint.h>
#include "stm32f7xx.h"

typedef struct {
    uint8_t seconds;
    uint8_t minutes;
    uint8_t hours;
    uint8_t weekday;
    uint8_t day;
    uint8_t month;
    uint16_t year;
} rtc_datetime_t;

void rtc_init(void);
void rtc_get_datetime(rtc_datetime_t *datetime);
void rtc_set_datetime(const rtc_datetime_t *datetime);

#endif // RTC_H