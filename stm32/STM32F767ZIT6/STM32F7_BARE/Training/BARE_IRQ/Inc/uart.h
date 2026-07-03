#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f7xx.h"

#define UART3_TX 8U // PD8 TX
#define UART3_RX 9U // PD9 RX

#define UART2_TX 5U // PD5 TX
#define UART2_RX 6U // PD6 RX

#define uart3_buffer_size 256
extern uint8_t uart3_data[uart3_buffer_size];

#define uart2_buffer_size 29
extern uint8_t uart2_data[uart2_buffer_size];

void uart3_rxtx_init(void);
void uart2_rxtx_init(void);
void uart_write(USART_TypeDef *USARTx, uint8_t data);
void uart_read(USART_TypeDef *USARTx, uint8_t *data);

#endif // UART_H