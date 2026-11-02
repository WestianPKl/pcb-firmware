#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "stm32c5xx.h"

#define SPI1_SCK 5U // PA5
#define SPI1_MISO 6U // PA6
#define SPI1_MOSI 7U // PA7
#define SPI1_CS 4U //PA4

typedef struct
{
    uint32_t frequency;
    uint8_t cpol;
    uint8_t cpha;
    uint8_t bit_order;
    uint8_t data_size;
} spi_config_t;

void spi1_init(void);
uint8_t spi1_config(const spi_config_t *config);
uint8_t spi1_transfer_byte(uint8_t tx, uint8_t *rx);;
uint8_t spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t length);

#endif // SPI_H