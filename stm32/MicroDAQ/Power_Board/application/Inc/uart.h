#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32c0xx.h"

#define UART1_TX_PIN 9U // PA9
#define UART1_RX_PIN 10U // PA10

extern volatile uint8_t uart1_tx_busy;
extern volatile uint16_t uart1_rx_length;
extern volatile uint8_t uart1_rx_ready;

#define uart1_buffer_size 256
extern uint8_t uart1_data[uart1_buffer_size];

void uart1_init(uint32_t baudrate);

void uart1_dma1_rx_start(uint8_t *dst, uint16_t len);
void uart1_dma1_tx_start(uint8_t *mem_addr, uint16_t len);

uint8_t uart1_write_dma(uint8_t *data, uint16_t len);
void uart1_receive_dma(uint8_t *dst, uint16_t len);

#endif // UART_H