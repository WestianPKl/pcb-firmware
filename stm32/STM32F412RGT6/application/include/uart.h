#ifndef UART_H
#define UART_H

#include <stdint.h>
#include "stm32f4xx.h"

void uart1_rxtx_init(void);
void uart2_rxtx_init(void);

#endif // UART_H