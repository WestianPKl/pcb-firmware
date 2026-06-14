#ifndef UART_H
#define UART_H

#include <stdint.h>
#include <stdbool.h>
#include "stm32f4xx.h"

void uart2_rxtx_init(void);
void uart2_send(const uint8_t *buf, uint32_t len);
bool uart2_rx_pop(uint8_t *out);

uint8_t crc8_atm(const uint8_t *data, uint32_t len);

#endif // UART_H