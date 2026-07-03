#ifndef RNG_H
#define RNG_H

#include <stdint.h>
#include "stm32f7xx.h"

uint8_t rng_get(uint32_t *out);

#endif // RNG_H