#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>

uint8_t ethernet_init(void);
const uint8_t *ethernet_get_mac(void);
uint8_t ethernet_tx(const uint8_t *frame, uint16_t length);
uint8_t ethernet_rx_poll(uint8_t **frame, uint16_t *length);
void ethernet_rx_release(void);

#endif // ETHERNET_H
