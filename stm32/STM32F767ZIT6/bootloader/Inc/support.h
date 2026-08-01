#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include "stm32f7xx.h"

typedef enum {
    RESET_POWER_ON,
    RESET_PIN,
    RESET_SOFTWARE,
    RESET_IWDG,
    RESET_WWDG,
    RESET_BROWNOUT,
    RESET_LOWPOWER,
    RESET_UNKNOWN
} reset_cause_t;

uint32_t apb1_pclk_hz(void);
uint32_t tim_apb1_clock_hz(void);
uint32_t apb2_pclk_hz(void);
uint32_t tim_apb2_clock_hz(void);

reset_cause_t get_reset_cause(void);
void dwt_init(void);
void delay_us(uint32_t us);
void delay_ms(uint32_t ms);

uint8_t crc8_atm(const uint8_t *data, uint32_t len);
uint16_t crc16_ccitt(const uint8_t *data, uint32_t len);

#endif // SUPPORT_H