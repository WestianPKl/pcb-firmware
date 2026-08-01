#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>

#define MAGIC 0xAAU
#define ADDRESS 0x01U
#define PROTOCOL_MAX_PAYLOAD 32U
#define PROTOCOL_FRAME_MAX (8U + PROTOCOL_MAX_PAYLOAD)
// REQ: [MAGIC] [LENGTH] [ADDRESS] [SEQUENCE] [COMMAND] [DATA...] [CRC_H] [CRC_L]
// RES: [MAGIC] [LENGTH] [ADDRESS] [SEQUENCE] [COMMAND] [STATUS] [DATA...] [CRC_H] [CRC_L]

typedef enum {
    PROTOCOL_STATUS_OK      = 0x00,
    PROTOCOL_STATUS_ERROR   = 0x01,
    PROTOCOL_STATUS_BUSY    = 0x02,
    PROTOCOL_STATUS_TIMEOUT = 0x03,
} protocol_status_t;

typedef struct {
    uint8_t magic;
    uint8_t length;
    uint8_t address;
    uint8_t sequence;
    uint8_t command;
    uint8_t data[PROTOCOL_MAX_PAYLOAD];
} protocol_request_t;

uint16_t protocol_process_request(const protocol_request_t *request, uint8_t *frame);


#endif // PROTOCOL_H
