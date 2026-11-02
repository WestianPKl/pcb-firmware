
#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32h5xx.h"

#define UART2_TX 5U // PD5 TX
#define UART2_RX 6U // PD6 RX

#define UART2_RX_BUFFER_SIZE 128U

void uart2_init(uint32_t baudrate);
uint8_t uart2_dma_write(const uint8_t *data, uint16_t length);
void uart2_process(void);
void uart2_rx_callback(const uint8_t *data, uint16_t length);

#endif // UART_H