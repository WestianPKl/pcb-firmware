#ifndef PCF8563T_H
#define PCF8563T_H

#include <stdint.h>
#include "stm32f7xx.h"
#include "rtc.h"

int pcf8563t_init(void);
int pcf8563t_set_datetime(const rtc_datetime_t *datetime);
int pcf8563t_get_datetime(rtc_datetime_t *datetime);

#endif // PCF8563T_H