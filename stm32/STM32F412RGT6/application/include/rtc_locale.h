
#ifndef RTC_LOCALE_H
#define RTC_LOCALE_H

#include <stdint.h>

void rtc_utc_to_warsaw(uint8_t *yy, uint8_t *mo, uint8_t *dd, uint8_t *wd,
                       uint8_t *hh, uint8_t *mi, uint8_t *ss);

#endif // RTC_LOCALE_H
