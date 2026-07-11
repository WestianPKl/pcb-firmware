#ifndef PCF8563T_H
#define PCF8563T_H

#include <stdint.h>
#include "stm32f7xx.h"
#include "rtc.h"

void pcf8563t_init(void);
void pcf8563t_set_datetime(const rtc_datetime_t *datetime);
void pcf8563t_get_datetime(rtc_datetime_t *datetime);

#endif // PCF8563T_H