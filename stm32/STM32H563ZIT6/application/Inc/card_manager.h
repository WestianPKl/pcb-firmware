
#ifndef CARD_MANAGER_H
#define CARD_MANAGER_H

#include <stdint.h>
#include "stm32h5xx.h"

#define SLOT_COUNT 3U

#define SYNC_PIN 8U // PC8

#define SLOT0_RESET_PIN 9U // PC9
#define SLOT0_FAULT_PIN 10U // PC10
#define SLOT0_DETECT_PIN 11U // PC11

#define SLOT1_RESET_PIN 2U // PE2
#define SLOT1_FAULT_PIN 4U // PE4
#define SLOT1_DETECT_PIN 5U // PE5

#define SLOT2_RESET_PIN 6U // PE6
#define SLOT2_FAULT_PIN 7U // PE7
#define SLOT2_DETECT_PIN 8U // PE8

#define CARD_TYPE_POWER          0x0001U
#define CARD_TYPE_SWITCH         0x0002U
#define CARD_TYPE_MEASUREMENT    0x0003U
#define CARD_TYPE_COMMUNICATION  0x0004U
#define CARD_TYPE_CONTROLLER     0x0005U

extern volatile uint8_t slot0_detect_changed;
extern volatile uint8_t slot0_fault_changed;

extern volatile uint8_t slot1_detect_changed;
extern volatile uint8_t slot1_fault_changed;

extern volatile uint8_t slot2_detect_changed;
extern volatile uint8_t slot2_fault_changed;

typedef struct
{
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint16_t year;
    uint8_t month;
    uint8_t day;
} fw_version_t;

typedef struct
{
    uint8_t  hw_major;
    uint8_t  hw_minor;
    uint16_t year;
    uint8_t  month;
    uint8_t  day;
    uint32_t serial_number;
    uint16_t card_type;
} board_info_t;

typedef struct
{
    uint16_t relay_mask;
} switch_card_status_t;

#define CARD_HEARTBEAT_MISSED_MAX  3U

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
    uint8_t online;
    uint8_t heartbeat_counter;
    uint8_t status_flags;
    uint16_t fault_flags;
    uint8_t reset_reason;
    uint8_t can_state;
    uint32_t uptime;
    uint8_t protocol_major;
    uint8_t protocol_minor;
    uint8_t heartbeat_seen;
    uint8_t missed_heartbeats;
} card_service_status_t;

typedef struct
{
    uint8_t detected;
    uint8_t fault;
    uint8_t identified;
    board_info_t board_info;
    fw_version_t fw_version;
    card_service_status_t service;
    switch_card_status_t switch_status;
} card_slot_t;

#define REQUEST_PROTOCOL_MAX_PAYLOAD 61U
#define RESPONSE_PROTOCOL_MAX_PAYLOAD 60U
#define PROTOCOL_REQUEST_FRAME_MAX (3U + REQUEST_PROTOCOL_MAX_PAYLOAD)
#define PROTOCOL_RESPONSE_FRAME_MAX (4U + RESPONSE_PROTOCOL_MAX_PAYLOAD)

typedef enum {
    CAN_PROTOCOL_STATUS_OK      = 0x00,
    CAN_PROTOCOL_STATUS_ERROR   = 0x01,
    CAN_PROTOCOL_STATUS_BUSY    = 0x02,
    CAN_PROTOCOL_STATUS_TIMEOUT = 0x03,
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

extern volatile card_slot_t card_slots[SLOT_COUNT];

void card_manager_init(void);
void card_manager_1s_process(void);
void card_manager_process(void);
void card_manager_can_receive(uint32_t can_id, uint8_t *can_data, uint8_t can_length);
uint8_t card_manager_get_relay_state(uint8_t slot, uint8_t relay);


#endif // CARD_MANAGER_H