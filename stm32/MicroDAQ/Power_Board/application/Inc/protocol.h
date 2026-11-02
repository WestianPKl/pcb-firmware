#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "support.h"

#define MAGIC                       0xAAU
#define POWER_BOARD_ADDRESS         0x22U

#define REQUEST_PROTOCOL_MAX_PAYLOAD    61U
#define RESPONSE_PROTOCOL_MAX_PAYLOAD   60U

#define PROTOCOL_REQUEST_FRAME_MAX      (7U + REQUEST_PROTOCOL_MAX_PAYLOAD)
#define PROTOCOL_RESPONSE_FRAME_MAX     (8U + RESPONSE_PROTOCOL_MAX_PAYLOAD)

#define CMD_PING                    0x00U
#define CMD_BOARD_INFO              0x10U
#define CMD_ADC_DATA                0x30U
#define CMD_INA_DATA                0x31U

// #define STATUS_READY           (1U << 0)
// #define STATUS_FAULT           (1U << 1)
// #define STATUS_WATCHDOG_RESET  (1U << 6)

typedef enum
{
    PROTOCOL_STATUS_OK      = 0x00U,
    PROTOCOL_STATUS_ERROR   = 0x01U,
    PROTOCOL_STATUS_BUSY    = 0x02U,
    PROTOCOL_STATUS_TIMEOUT = 0x03U,
} protocol_status_t;


typedef struct
{
    uint8_t magic;
    uint8_t address;
    uint8_t command;
    uint8_t sequence;
    uint8_t length;
    uint8_t data[REQUEST_PROTOCOL_MAX_PAYLOAD];
} protocol_request_t;


typedef struct
{
    uint8_t magic;
    uint8_t address;
    uint8_t command;
    uint8_t sequence;
    uint8_t status;
    uint8_t length;
    uint8_t data[RESPONSE_PROTOCOL_MAX_PAYLOAD];
} protocol_response_t;

void protocol_service_init(const reset_reason_t *reason);
uint8_t protocol_parse_request(const uint8_t *frame, uint16_t frame_length, protocol_request_t *request);
void protocol_process_request(const protocol_request_t *request,protocol_response_t *response);
uint16_t protocol_build_response(const protocol_response_t *response, uint8_t *frame);

#endif // PROTOCOL_H