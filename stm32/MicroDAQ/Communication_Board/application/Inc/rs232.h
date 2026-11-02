#ifndef RS232_H
#define RS232_H

#include <stdint.h>
#include "support.h"
#include "stm32c5xx.h"

#define RS232_RX_BUFFER_SIZE 55U
#define RS232_TX_BUFFER_SIZE 55U

#define UART4_TX 0U // PA0
#define UART4_RX 1U // PA1

void rs232_init(uint32_t baudrate);
uint8_t rs232_config(const serial_config_t *config);
uint8_t rs232_dma_write(const uint8_t *data, uint16_t length);
void rs232_process(void);
void rs232_tx_callback(uint8_t status);
void rs232_rx_callback(const uint8_t *data, uint16_t length);

#endif // RS232_H