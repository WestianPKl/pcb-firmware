#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "stm32f7xx.h"

#define SPI1_SCK 5U // PA5
#define SPI1_MISO 6U // PA6
#define SPI1_MOSI 7U // PA7
#define SPI1_CS 12U //PF12

#define SPI3_SCK 3U // PB3
#define SPI3_MISO 4U // PB4
#define SPI3_MOSI 5U // PB5
#define SPI3_CS 6U // PB6

void spi1_cs_high(void);
void spi1_cs_low(void);
void spi3_cs_high(void);
void spi3_cs_low(void);

void spi1_init(void);
void spi3_init(void);

uint8_t spi1_xfer(uint8_t out);
void    spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len);
void    spi1_write_reg(uint8_t reg, uint8_t val);
uint8_t spi1_read_reg(uint8_t reg);

#endif // SPI_H