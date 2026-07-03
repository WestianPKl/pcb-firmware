#ifndef APP_FLAGS_H
#define APP_FLAGS_H

#include <stdint.h>
#include "FreeRTOS.h"
#include "event_groups.h"

#define EVT_RTC_REINIT         (1UL << 1)
#define EVT_BME280_REINIT      (1UL << 2)

#define EVT_SHT40_ERROR        (1UL << 3)
#define EVT_BME280_ERROR       (1UL << 4)

#define EVT_EXT_RTC_PRESENT    (1UL << 5)
#define EVT_SHT40_PRESENT      (1UL << 6)
#define EVT_BME280_PRESENT     (1UL << 7)
#define EVT_ADC_PRESENT        (1UL << 8)

#define EVT_ALL_PERIPHERAL_BITS ( \
    EVT_EXT_RTC_PRESENT | EVT_SHT40_PRESENT | EVT_BME280_PRESENT | EVT_ADC_PRESENT)

extern EventGroupHandle_t appEvents;

void app_flags_apply(uint8_t cfg_flags);

#endif // APP_FLAGS_H