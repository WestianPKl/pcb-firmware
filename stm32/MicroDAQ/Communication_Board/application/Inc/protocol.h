
#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "support.h"
#include "can.h"

#define REQUEST_PROTOCOL_MAX_PAYLOAD 61U
#define RESPONSE_PROTOCOL_MAX_PAYLOAD 60U
#define MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD 56U
#define PROTOCOL_REQUEST_FRAME_MAX (3U + REQUEST_PROTOCOL_MAX_PAYLOAD)
#define PROTOCOL_RESPONSE_FRAME_MAX (4U + RESPONSE_PROTOCOL_MAX_PAYLOAD)

typedef enum {
    PROTOCOL_STATUS_OK          = 0x00,
    PROTOCOL_STATUS_ERROR       = 0x01,
    PROTOCOL_STATUS_BUSY        = 0x02,
    PROTOCOL_STATUS_TIMEOUT     = 0x03,
    PROTOCOL_STATUS_MULTIPART   = 0x04,
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

typedef struct
{
    uint8_t command;
    uint8_t sequence;
    uint8_t status;
    uint8_t length;
    uint16_t count;
    uint16_t package;
    uint8_t data[MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD];
} can_protocol_multipart_response_t;

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

// DUT CAN states
#define COMM_CAN_ACTIVE    0U
#define COMM_CAN_WARNING   1U
#define COMM_CAN_PASSIVE   2U
#define COMM_CAN_BUS_OFF   3U

// Communication interface busy flags
#define COMM_UART_BUSY     (1U << 0)
#define COMM_RS232_BUSY    (1U << 1)
#define COMM_RS485_BUSY    (1U << 2)
#define COMM_LIN_BUSY      (1U << 3)
#define COMM_I2C_BUSY      (1U << 4)
#define COMM_SPI_BUSY      (1U << 5)
#define COMM_CAN_BUSY      (1U << 6)

// Communication interface error flags
#define COMM_UART_ERROR        (1U << 0)
#define COMM_RS232_ERROR       (1U << 1)
#define COMM_RS485_ERROR       (1U << 2)
#define COMM_LIN_ERROR         (1U << 3)
#define COMM_I2C_ERROR         (1U << 4)
#define COMM_SPI_ERROR         (1U << 5)
#define COMM_CAN_ERROR         (1U << 6)

// Communication status structure
typedef struct
{
    uint8_t dut_can_state;
    uint8_t interface_status;
    uint8_t error_flags;
} communication_status_t;

extern volatile communication_status_t communication_status;

#define CMD_PING                    0x00U
#define CMD_IDENTIFY                0x01U
#define CMD_GET_DEVICE_INFO         0x02U
#define CMD_BOARD_INFO              0x10U
#define CMD_I2C_CONFIG              0x30U
#define CMD_I2C_WRITE               0x31U
#define CMD_I2C_READ                0x32U
#define CMD_I2C_REG_WRITE           0x33U
#define CMD_I2C_REG_READ            0x34U
#define CMD_SPI_CONFIG              0x35U
#define CMD_SPI_TRANSFER_BYTE       0x36U
#define CMD_SPI_TRANSFER_WORD       0x37U
#define CMD_UART_CONFIG             0x38U
#define CMD_UART_WRITE              0x39U
#define CMD_UART_READ               0x3AU
#define CMD_RS232_CONFIG            0x3BU
#define CMD_RS232_WRITE             0x3CU
#define CMD_RS232_READ              0x3DU
#define CMD_RS485_CONFIG            0x3EU
#define CMD_RS485_WRITE             0x3FU
#define CMD_RS485_READ              0x40U
#define CMD_CAN_CONFIG              0x41U
#define CMD_CAN_SEND_STD            0x42U
#define CMD_CAN_SEND_FD             0x43U
#define CMD_CAN_RECEIVE             0x44U
#define CMD_MULTIPART_TEST          0xFFU

void protocol_service_init(const reset_reason_t *reason);
uint8_t can_protocol_process_request(const can_protocol_request_t *request, can_protocol_response_t *frame);
void can_process_response(const can_protocol_request_t *request, uint8_t can_length);
void can_send_100ms_frames(void);
void can_send_1s_frames(void);
void identify_process(void);
void can2_tx_callback(uint8_t tx_status);
void can2_rx_callback(uint32_t id, can_id_type_t id_type, const uint8_t *data, uint8_t length, uint8_t fd, uint8_t brs);
void can2_tx_process(void);
void multipart_tx_process(void);

#endif // PROTOCOL_H
