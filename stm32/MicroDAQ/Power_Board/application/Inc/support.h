
#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include "stm32c0xx.h"

#define RESET_REASON_PIN        (1U << 0)
#define RESET_REASON_BOR        (1U << 1)
#define RESET_REASON_SOFT       (1U << 2)
#define RESET_REASON_IWDG       (1U << 3)
#define RESET_REASON_WWDG       (1U << 4)
#define RESET_REASON_LOW_POWER  (1U << 5)

typedef struct
{
    uint8_t reset_pin;
    uint8_t reset_bor;
    uint8_t reset_soft;
    uint8_t reset_iwdg;
    uint8_t reset_wwdg;
    uint8_t reset_low_power;
} reset_reason_t;

uint32_t apb_pclk_hz(void);
uint32_t tim_apb_clock_hz(void);

void reset_reason_get(reset_reason_t *r);
uint8_t reset_reason_pack(const reset_reason_t *r);

uint8_t iwdg_init(void);
void iwdg_refresh(void);

uint8_t crc8_atm(const uint8_t *data, uint32_t len);
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len);
uint32_t crc32(const uint8_t *data, uint32_t len);

#endif // SUPPORT_H
