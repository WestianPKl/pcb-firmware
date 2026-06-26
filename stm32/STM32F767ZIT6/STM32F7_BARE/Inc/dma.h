#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include "stm32f7xx.h"

#define SPI_DMA_TIMEOUT 200000U

extern volatile uint8_t uart3_tx_busy;
extern volatile uint8_t uart2_tx_busy;

extern volatile uint8_t spi1_dma_rx_done;
extern volatile uint8_t spi1_dma_tx_done;

extern volatile uint8_t spi3_dma_rx_done;
extern volatile uint8_t spi3_dma_tx_done;

void dma1_init(void);
void dma2_init(void);

void adc1_dma2_config(uint8_t continuous, uint16_t *dst, uint16_t len);

void uart3_dma1_rx_start(uint8_t *dst, uint16_t len);
void uart3_dma1_tx_start(uint8_t *mem_addr, uint16_t len);

uint8_t uart3_write_dma(uint8_t *data, uint16_t len);
void uart3_receive_dma(uint8_t *dst, uint16_t len);

void uart2_dma1_rx_start(uint8_t *dst, uint16_t len);
void uart2_dma1_tx_start(uint8_t *mem_addr, uint16_t len);

uint8_t uart2_write_dma(uint8_t *data, uint16_t len);
void uart2_receive_dma(uint8_t *dst, uint16_t len);

void spi1_dma_rx_init(void);
void spi1_dma_tx_init(void);
void spi1_dma_transfer(uint32_t msg_to_send, uint32_t msg_len);
void spi1_dma_receive(uint32_t received_msg, uint32_t msg_len);

void spi3_dma_rx_init(void);
void spi3_dma_tx_init(void);
void spi3_dma_transfer(uint32_t msg_to_send, uint32_t msg_len);
void spi3_dma_receive(uint32_t received_msg, uint32_t msg_len);

void spi1_dma_xfer(const uint8_t *tx, uint8_t *rx, uint32_t len);

void i2c2_dma_tx_init(void);
void i2c2_dma_rx_init(void);
void i2c2_dma_tx_start(uint32_t buf, uint16_t len);
void i2c2_dma_rx_start(uint32_t buf, uint16_t len);

#endif // DMA_H