#ifndef PROTOCOL_H
#define PROTOCOL_H

#include <stdint.h>
#include "support.h"

#define PROTOCOL_MAGIC              0xAAU
#define PROTOCOL_MAX_PAYLOAD        256U

#define PROTOCOL_REQUEST_MAX        (7U + PROTOCOL_MAX_PAYLOAD)
#define PROTOCOL_RESPONSE_MAX       (8U + PROTOCOL_MAX_PAYLOAD)

typedef enum
{
    PROTOCOL_STATUS_OK          = 0x40U,
    PROTOCOL_STATUS_MULTIPART   = 0x50U,
    PROTOCOL_STATUS_ERROR       = 0x7FU,
    PROTOCOL_STATUS_BUSY        = 0x41U,
    PROTOCOL_STATUS_TIMEOUT     = 0x42U
} protocol_status_t;

typedef enum
{
    PROTOCOL_CMD_PING                   = 0x00U,
    PROTOCOL_CMD_GET_CONTROLLER_INFO    = 0x01U,
    PROTOCOL_CMD_GET_CARDS_INFO         = 0x10U,
    PROTOCOL_CMD_IDENTIFY_CARD          = 0x11U,
    PROTOCOL_CMD_GET_RELAY              = 0x20U,
    PROTOCOL_CMD_SET_RELAY              = 0x21U,
    PROTOCOL_CMD_GET_DIGITAL_OUTPUT     = 0x22U,
    PROTOCOL_CMD_SET_DIGITAL_OUTPUT     = 0x23U,
    PROTOCOL_CMD_GET_PWM                = 0x24U,
    PROTOCOL_CMD_SET_PWM                = 0x25U,
    PROTOCOL_CMD_GET_POWER_CARD_INFO    = 0x30U,
    PROTOCOL_CMD_ADC_DATA               = 0x31U,
    PROTOCOL_CMD_INA_DATA               = 0x32U,
    PROTOCOL_CMD_I2C_CONFIG             = 0x40U,
    PROTOCOL_CMD_I2C_WRITE              = 0x41U,
    PROTOCOL_CMD_I2C_READ               = 0x42U,
    PROTOCOL_CMD_I2C_REG_WRITE          = 0x43U,
    PROTOCOL_CMD_I2C_REG_READ           = 0x44U,
    PROTOCOL_CMD_SPI_CONFIG             = 0x45U,
    PROTOCOL_CMD_SPI_TRANSFER_BYTE      = 0x46U,
    PROTOCOL_CMD_SPI_TRANSFER_WORD      = 0x47U,
    PROTOCOL_CMD_UART_CONFIG            = 0x48U,
    PROTOCOL_CMD_UART_WRITE             = 0x49U,
    PROTOCOL_CMD_UART_READ              = 0x4AU,
    PROTOCOL_CMD_RS232_CONFIG           = 0x4BU,
    PROTOCOL_CMD_RS232_WRITE            = 0x4CU,
    PROTOCOL_CMD_RS232_READ             = 0x4DU,
    PROTOCOL_CMD_RS485_CONFIG           = 0x4EU,
    PROTOCOL_CMD_RS485_WRITE            = 0x4FU,
    PROTOCOL_CMD_RS485_READ             = 0x50U,
    PROTOCOL_CMD_CAN_CONFIG             = 0x51U,
    PROTOCOL_CMD_CAN_STD_SEND           = 0x52U,
    PROTOCOL_CMD_CAN_FD_SEND            = 0x53U,
    PROTOCOL_CMD_CAN_RECEIVE            = 0x54U,
    PROTOCOL_CMD_MULTIPART_TEST         = 0xFFU
} protocol_command_t;

typedef struct
{
    uint16_t length;
    uint8_t sequence;
    uint8_t command;
    uint8_t data[PROTOCOL_MAX_PAYLOAD];
} protocol_request_t;

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

#define PROTOCOL_RESPONSE_PENDING  0U

uint16_t protocol_process_request(const protocol_request_t *request, uint8_t *frame);
void protocol_process(void);
void tcp_protocol_rx(const uint8_t *data, uint16_t length);
void protocol_service_init(const reset_reason_t *reason);

#endif