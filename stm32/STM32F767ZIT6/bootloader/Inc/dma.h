#ifndef DMA_H
#define DMA_H

#include <stdint.h>
#include "stm32f7xx.h"

extern volatile uint8_t uart3_tx_busy;
extern volatile uint8_t uart2_tx_busy;

void dma1_init(void);
void dma1_stop_all(void);

void uart3_dma1_rx_start(uint8_t *dst, uint16_t len);
void uart3_dma1_tx_start(uint8_t *mem_addr, uint16_t len);

uint8_t uart3_write_dma(uint8_t *data, uint16_t len);
void uart3_receive_dma(uint8_t *dst, uint16_t len);

void uart2_dma1_rx_start(uint8_t *dst, uint16_t len);
void uart2_dma1_tx_start(uint8_t *mem_addr, uint16_t len);

uint8_t uart2_write_dma(uint8_t *data, uint16_t len);
void uart2_receive_dma(uint8_t *dst, uint16_t len);

#endif // DMA_H