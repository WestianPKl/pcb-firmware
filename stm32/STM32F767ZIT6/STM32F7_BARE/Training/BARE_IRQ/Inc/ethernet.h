#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include "stm32f7xx.h"

extern volatile uint8_t eth_rx_ready;

void ethernet_init(void);

uint8_t eth_send_frame(const uint8_t *data, uint16_t len);
uint16_t eth_recv_frame(uint8_t *buf, uint16_t maxlen);
uint8_t  eth_link_up(void);

#endif // ETHERNET_H
