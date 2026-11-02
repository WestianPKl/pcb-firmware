
#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include "stm32h5xx.h"

extern volatile uint32_t service_uptime;

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

void clock_init_250mhz(void);
uint32_t apb1_pclk_hz(void);
uint32_t tim_apb1_clock_hz(void);

uint32_t apb2_pclk_hz(void);
uint32_t tim_apb2_clock_hz(void);

uint8_t crc8_atm(const uint8_t *data, uint32_t len);
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len);
uint32_t crc32(const uint8_t *data, uint32_t len);

void reset_reason_get(reset_reason_t *r);
uint8_t reset_reason_pack(const reset_reason_t *r);

uint8_t iwdg_init(void);
void iwdg_refresh(void);

void wwdg_init(void);
void wwdg_refresh(void);
uint8_t wwdg_refresh_if_window(void);

uint8_t rng_init(void);
uint8_t rng_get(uint32_t *value);

void crc_init(void);
uint32_t crc32_calculate(const uint32_t *data, uint32_t length);

uint8_t vrefbuf_init(void);

uint8_t dts_init(void);
uint8_t dts_get_temperature(int32_t *temperature);

void cordic_init(void);
uint8_t cordic_cos_sin(int32_t angle,
                       int32_t *cos_result,
                       int32_t *sin_result);

void pwr_enter_stop(void);

#endif // SUPPORT_H
