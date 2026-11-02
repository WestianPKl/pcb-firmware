#ifndef RS485_H
#define RS485_H

#include <stdint.h>
#include "support.h"
#include "stm32c5xx.h"

#define RS485_RX_BUFFER_SIZE 55U
#define RS485_TX_BUFFER_SIZE 55U

#define UART2_TX 0U // PB0
#define UART2_RX 1U // PB1
#define RS485_DE 9U // PB9

void rs485_init(uint32_t baudrate);
uint8_t rs485_config(const serial_config_t *config);
void rs485_termination_on(void);
void rs485_termination_off(void);
uint8_t rs485_dma_write(const uint8_t *data, uint16_t length);
void rs485_process(void);
void rs485_tx_callback(uint8_t status);
void rs485_rx_callback(const uint8_t *data, uint16_t length);

#endif // RS485_H