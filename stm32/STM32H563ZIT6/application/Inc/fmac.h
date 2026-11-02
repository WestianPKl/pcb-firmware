#ifndef FMAC_H
#define FMAC_H

#include <stdint.h>
#include "stm32h5xx.h"

uint8_t fmac_init(void);
uint8_t fmac_process_sample(int16_t input,
                            int16_t *output);

#endif // FMAC_H
