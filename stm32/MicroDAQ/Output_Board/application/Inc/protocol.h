
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "support.h"

#define REQUEST_PROTOCOL_MAX_PAYLOAD 61U
#define RESPONSE_PROTOCOL_MAX_PAYLOAD 60U
#define PROTOCOL_REQUEST_FRAME_MAX (3U + REQUEST_PROTOCOL_MAX_PAYLOAD)
#define PROTOCOL_RESPONSE_FRAME_MAX (4U + RESPONSE_PROTOCOL_MAX_PAYLOAD)

typedef enum {
    PROTOCOL_STATUS_OK      = 0x00,
    PROTOCOL_STATUS_ERROR   = 0x01,
    PROTOCOL_STATUS_BUSY    = 0x02,
    PROTOCOL_STATUS_TIMEOUT = 0x03,
} can_protocol_status_t;

typedef struct
{
    uint8_t command;
    uint8_t sequence;
    uint8_t length;
    uint8_t data[REQUEST_PROTOCOL_MAX_PAYLOAD];
} can_protocol_request_t;

typedef struct
{
    uint8_t command;
    uint8_t sequence;
    uint8_t status;
    uint8_t length;
    uint8_t data[RESPONSE_PROTOCOL_MAX_PAYLOAD];
} can_protocol_response_t;

#define PROTOCOL_VERSION_MAJOR  1U
#define PROTOCOL_VERSION_MINOR  0U

#define STATUS_READY           (1U << 0)
#define STATUS_FAULT           (1U << 1)
#define STATUS_CAN_WARNING     (1U << 2)
#define STATUS_CAN_PASSIVE     (1U << 3)
#define STATUS_CAN_BUS_OFF     (1U << 4)
#define STATUS_SYNC_SEEN       (1U << 5)
#define STATUS_WATCHDOG_RESET  (1U << 6)

#define SERVICE_CAN_ACTIVE     0x00U
#define SERVICE_CAN_WARNING    0x01U
#define SERVICE_CAN_PASSIVE    0x02U
#define SERVICE_CAN_BUS_OFF    0x03U

typedef struct
{
    uint8_t heartbeat_counter;
    uint8_t status_flags;
    uint16_t fault_flags;
    uint8_t reset_reason;
    uint8_t can_state;
    uint32_t uptime;
    uint8_t protocol_major;
    uint8_t protocol_minor;
} service_status_t;

void protocol_service_init(const reset_reason_t *reason);
uint8_t can_protocol_process_request(const can_protocol_request_t *request, can_protocol_response_t *frame);
void can_process_response(const can_protocol_request_t *request, uint8_t can_length);
void can_send_100ms_frames(void);
void can_send_1s_frames(void);
void identify_process(void);

#endif // PROTOCOL_H
