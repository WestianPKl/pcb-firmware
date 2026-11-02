
#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include "stm32h5xx.h"

extern volatile uint8_t reset_pin;
extern volatile uint8_t reset_bor;
extern volatile uint8_t reset_soft;
extern volatile uint8_t reset_iwdg;
extern volatile uint8_t reset_wwdg;
extern volatile uint8_t reset_low_power;

void clock_init_250mhz(void);
uint32_t apb1_pclk_hz(void);
uint32_t tim_apb1_clock_hz(void);

uint32_t apb2_pclk_hz(void);
uint32_t tim_apb2_clock_hz(void);

uint8_t crc8_atm(const uint8_t *data, uint32_t len);
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len);

void reset_reason_read(void);
uint32_t reset_reason_get(void);

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
