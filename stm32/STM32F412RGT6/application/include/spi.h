#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "stm32f4xx.h"

void spi1_init(void);
void spi1_cs_low(void);
void spi1_cs_high(void);

#endif // SPI_H