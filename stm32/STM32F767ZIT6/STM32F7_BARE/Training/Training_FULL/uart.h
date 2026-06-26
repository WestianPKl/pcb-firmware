#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f7xx.h"

#define UART3_TX (1U << 8)
#define UART3_RX (1U << 9)

void uart3_rxtx_init(void);
void uart_write(USART_TypeDef *USARTx, uint8_t data);
void uart_read(USART_TypeDef *USARTx, uint8_t *data);

#endif // UART_H