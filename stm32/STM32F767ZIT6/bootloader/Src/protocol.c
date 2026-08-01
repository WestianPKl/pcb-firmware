#include "protocol.h"
#include "support.h"

uint16_t protocol_process_request(const protocol_request_t *request, uint8_t *frame)
{
    uint8_t length = 0U;
    uint8_t status;

    frame[0] = MAGIC;
    frame[2] = ADDRESS;
    frame[3] = request->sequence;
    frame[4] = request->command;

    switch (request->command) {
        case 0x00:
            frame[6] = 0x11;
            frame[7] = 0x22;
            frame[8] = 0x33;
            length = 3U;
            status = PROTOCOL_STATUS_OK;
            break;
        default:
            status = PROTOCOL_STATUS_ERROR;
            break;
    }

    frame[1] = length;
    frame[5] = status;

    uint16_t crc = crc16_ccitt(&frame[1], 5U + length);
    frame[6U + length] = (uint8_t)(crc >> 8);
    frame[7U + length] = (uint8_t)(crc & 0xFFU);

    return (uint16_t)(8U + length);
}