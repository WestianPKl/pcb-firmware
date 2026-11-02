#ifndef CONTROLLER_TCP_H
#define CONTROLLER_TCP_H

#include <stdint.h>

#define TCP_MAGIC 0xAAU
#define TCP_PROTOCOL_MAX_PAYLOAD 32U
#define TCP_PROTOCOL_FRAME_MAX (8U + TCP_PROTOCOL_MAX_PAYLOAD)

typedef enum {
    TCP_PROTOCOL_STATUS_OK      = 0x00,
    TCP_PROTOCOL_STATUS_ERROR   = 0x01,
    TCP_PROTOCOL_STATUS_BUSY    = 0x02,
    TCP_PROTOCOL_STATUS_TIMEOUT = 0x03,
} tcp_protocol_status_t;

typedef struct {
    uint8_t magic;
    uint16_t length;
    uint8_t sequence;
    uint8_t command;
    uint8_t data[TCP_PROTOCOL_MAX_PAYLOAD];
} tcp_protocol_request_t;

uint8_t controller_tcp_init(void);

#endif // CONTROLLER_TCP_H