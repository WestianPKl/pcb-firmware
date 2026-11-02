#ifndef CONTROLLER_TCP_H
#define CONTROLLER_TCP_H

#include <stdint.h>

uint8_t controller_tcp_init(void);
uint8_t controller_tcp_send(const uint8_t *data, uint16_t length);

#endif // CONTROLLER_TCP_H