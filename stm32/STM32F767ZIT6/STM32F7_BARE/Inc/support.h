#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>
#include "stm32f7xx.h"

uint32_t apb1_pclk_hz(void);
uint32_t tim_apb1_clock_hz(void);
uint32_t apb2_pclk_hz(void);
uint32_t tim_apb2_clock_hz(void);

#endif // SUPPORT_H