#include "app_flags.h"

EventGroupHandle_t appEvents;

void app_flags_apply(uint8_t cfg_flags)
{
    EventBits_t bits_to_set = 1;

    bits_to_set |= EVT_EXT_RTC_PRESENT;
    bits_to_set |= EVT_SHT40_PRESENT;
    bits_to_set |= EVT_BME280_PRESENT;
    bits_to_set |= EVT_ADC_PRESENT;

    xEventGroupClearBits(appEvents, EVT_ALL_PERIPHERAL_BITS);
    xEventGroupSetBits(appEvents, bits_to_set);
}