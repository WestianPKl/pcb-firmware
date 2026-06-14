#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include "stm32f4xx.h"

void dma1_init(void);
void dma2_init(void);
void dma2_adc1_config(uint8_t circular, uint16_t *dst, uint16_t len);

void dma1_uart2_rx_config(uint8_t *dst, uint16_t len);
void dma1_uart2_tx_start(uint8_t *mem_addr, uint16_t len);

void dma2_uart1_rx_config(uint8_t *dst, uint16_t len);
void dma2_uart1_tx_start(uint8_t *mem_addr, uint16_t len);

void dma_i2c1_rx_init(void);
void dma_i2c1_tx_init(void);
void dma_i2c1_rx_start(uint32_t dst, uint16_t len);
void dma_i2c1_tx_start(uint32_t src, uint16_t len);
void dma_i2c1_abort(void);

void dma_spi1_rx_init(void);
void dma_spi1_tx_init(void);
void dma_spi1_transfer(uint32_t msg_to_send, uint32_t msg_len);
void dma_spi1_receive(uint32_t received_msg, uint32_t msg_len);

#endif // DMA_H