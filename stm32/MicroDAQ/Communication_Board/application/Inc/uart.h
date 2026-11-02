#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "support.h"
#include "stm32c5xx.h"

#define UART5_RX_BUFFER_SIZE 55U
#define UART5_TX_BUFFER_SIZE 55U

#define UART5_TX 6U // PB6
#define UART5_RX 5U // PB5

void uart5_init(uint32_t baudrate);
uint8_t uart5_config(const serial_config_t *config);
uint8_t uart5_dma_write(const uint8_t *data, uint16_t length);
void uart5_process(void);
void uart5_tx_callback(uint8_t status);
void uart5_rx_callback(const uint8_t *data, uint16_t length);

#endif // UART_H