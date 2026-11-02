#ifndef SPI_H
#define SPI_H

#include <stdint.h>
#include "stm32h5xx.h"

#define SPI1_SCK 5U // PA5
#define SPI1_MISO 6U // PA6
#define SPI1_MOSI 7U // PD7
#define SPI1_CS 12U //PF12
#define SPI1_CS_BME 0U // PE0

#define SPI2_SCK 10U // PB10
#define SPI2_MISO 2U // PC2
#define SPI2_MOSI 3U // PC3
#define SPI2_CS 12U // PB12

extern volatile uint8_t spi1_dma_busy;
extern volatile uint8_t spi1_dma_done;
extern volatile uint8_t spi1_dma_error;

void spi1_init(uint8_t dma_enable);
uint8_t spi1_transfer_byte(uint8_t tx, uint8_t *rx);;
uint8_t spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t length, uint8_t cs_bme);
uint8_t spi1_dma_transfer(const uint8_t *tx, uint8_t *rx, uint16_t length, uint8_t cs_bme);

void spi2_init(void);
void spi2_process(void);

#endif // SPI_H
