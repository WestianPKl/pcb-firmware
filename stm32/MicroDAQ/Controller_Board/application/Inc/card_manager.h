
#ifndef CARD_MANAGER_H
#define CARD_MANAGER_H

#include <stdint.h>
#include "stm32h5xx.h"
#include "version.h"

#define SLOT_COUNT_MAX 3U

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

extern volatile uint8_t slot0_detect_changed;
extern volatile uint8_t slot0_fault_changed;

extern volatile uint8_t slot1_detect_changed;
extern volatile uint8_t slot1_fault_changed;

extern volatile uint8_t slot2_detect_changed;
extern volatile uint8_t slot2_fault_changed;

extern uint8_t slot_count;

typedef struct
{
    uint16_t relay_mask;
} relay_status_t;

typedef struct
{
    uint16_t output_mask;
} digital_output_status_t;

typedef struct
{
    uint16_t pwm_mask;
    uint8_t duty_cycle[4];
} pwm_status_t;

#define MAGIC                       0xAAU
#define POWER_BOARD_ADDRESS         0x22U

#define UART_CMD_PING                    0x00U
#define UART_CMD_BOARD_INFO              0x10U
#define UART_CMD_ADC_DATA                0x30U
#define UART_CMD_INA_DATA                0x31U

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
    relay_status_t relay_status;
    digital_output_status_t digital_output_status;
    pwm_status_t pwm_status;
} card_slot_t;

typedef struct
{
    uint16_t vin;
    uint16_t v12;
    uint16_t v5;
    uint16_t v33;
    int32_t current_ma;
    uint32_t voltage_mv;
    uint32_t power_mw;
    int32_t shunt_uv_x10;
    uint8_t adc_valid;
    uint8_t ina_valid;
    uint32_t adc_updated_ms;
    uint32_t ina_updated_ms;
} power_card_t;

#define POWER_DATA_TIMEOUT_MS 5000U

#define REQUEST_PROTOCOL_MAX_PAYLOAD 61U
#define RESPONSE_PROTOCOL_MAX_PAYLOAD 60U
#define MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD 56U
#define PROTOCOL_REQUEST_FRAME_MAX (3U + REQUEST_PROTOCOL_MAX_PAYLOAD)
#define PROTOCOL_RESPONSE_FRAME_MAX (4U + RESPONSE_PROTOCOL_MAX_PAYLOAD)

typedef enum {
    CAN_PROTOCOL_STATUS_OK          = 0x00,
    CAN_PROTOCOL_STATUS_ERROR       = 0x01,
    CAN_PROTOCOL_STATUS_BUSY        = 0x02,
    CAN_PROTOCOL_STATUS_TIMEOUT     = 0x03,
    CAN_PROTOCOL_STATUS_MULTIPART   = 0x04,
} can_protocol_status_t;

typedef enum {
    UART_PROTOCOL_STATUS_OK      = 0x00,
    UART_PROTOCOL_STATUS_ERROR   = 0x01,
    UART_PROTOCOL_STATUS_BUSY    = 0x02,
    UART_PROTOCOL_STATUS_TIMEOUT = 0x03,
} uart_protocol_status_t;

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

typedef struct
{
    uint8_t magic;
    uint8_t address;
    uint8_t command;
    uint8_t sequence;
    uint8_t length;
    uint8_t data[REQUEST_PROTOCOL_MAX_PAYLOAD];
} protocol_uart_request_t;

typedef struct
{
    uint8_t magic;
    uint8_t address;
    uint8_t command;
    uint8_t sequence;
    uint8_t status;
    uint8_t length;
    uint8_t data[RESPONSE_PROTOCOL_MAX_PAYLOAD];
} protocol_uart_response_t;

typedef struct
{
    uint8_t active;
    uint8_t slot;
    uint8_t sequence;
    uint8_t command;
    uint32_t start_tick;
} card_transaction_t;

typedef struct
{
    uint8_t ready;
    uint8_t sequence;
    uint8_t command;
    uint8_t status;
    uint8_t length;
    uint8_t data[RESPONSE_PROTOCOL_MAX_PAYLOAD];
} card_transaction_response_t;

typedef struct
{
    uint8_t ready;
    uint8_t sequence;
    uint8_t command;
    uint8_t status;
    uint16_t length;
    uint16_t count;
    uint16_t package;
    uint8_t data[1024];
} card_transaction_multipart_response_t;

#define CARD_TRANSACTION_TIMEOUT_MS    1000U

extern volatile card_slot_t card_slots[SLOT_COUNT_MAX];
extern volatile card_slot_t power_card;
extern volatile power_card_t power_card_data;

typedef enum
{
    CAN_PROTOCOL_CMD_PING                   = 0x00U,
    CAN_PROTOCOL_CMD_IDENTIFY_CARD          = 0x01U,
    CAN_PROTOCOL_CMD_GET_CARD_INFO          = 0x02U,
    CAN_PROTOCOL_CMD_GET_RELAY              = 0x10U,
    CAN_PROTOCOL_CMD_SET_RELAY              = 0x11U,
    CAN_PROTOCOL_CMD_GET_DIGITAL_OUTPUT     = 0x12U,
    CAN_PROTOCOL_CMD_SET_DIGITAL_OUTPUT     = 0x13U,
    CAN_PROTOCOL_CMD_GET_PWM                = 0x14U,
    CAN_PROTOCOL_CMD_SET_PWM                = 0x15U,
    CAN_PROTOCOL_CMD_I2C_CONFIG             = 0x30U,
    CAN_PROTOCOL_CMD_I2C_WRITE              = 0x31U,
    CAN_PROTOCOL_CMD_I2C_READ               = 0x32U,
    CAN_PROTOCOL_CMD_I2C_REG_WRITE          = 0x33U,
    CAN_PROTOCOL_CMD_I2C_REG_READ           = 0x34U,
    CAN_PROTOCOL_CMD_SPI_CONFIG             = 0x35U,
    CAN_PROTOCOL_CMD_SPI_TRANSFER_BYTE      = 0x36U,
    CAN_PROTOCOL_CMD_SPI_TRANSFER_WORD      = 0x37U,
    CAN_PROTOCOL_CMD_UART_CONFIG            = 0x38U,
    CAN_PROTOCOL_CMD_UART_WRITE             = 0x39U,
    CAN_PROTOCOL_CMD_UART_READ              = 0x3AU,
    CAN_PROTOCOL_CMD_RS232_CONFIG           = 0x3BU,
    CAN_PROTOCOL_CMD_RS232_WRITE            = 0x3CU,
    CAN_PROTOCOL_CMD_RS232_READ             = 0x3DU,
    CAN_PROTOCOL_CMD_RS485_CONFIG           = 0x3EU,
    CAN_PROTOCOL_CMD_RS485_WRITE            = 0x3FU,
    CAN_PROTOCOL_CMD_RS485_READ             = 0x40U,
    CAN_PROTOCOL_CMD_CAN_CONFIG             = 0x41U,
    CAN_PROTOCOL_CMD_CAN_STD_SEND           = 0x42U,
    CAN_PROTOCOL_CMD_CAN_FD_SEND            = 0x43U,
    CAN_PROTOCOL_CMD_CAN_RECEIVE            = 0x44U,
    CAN_PROTOCOL_CMD_MULTIPART_TEST         = 0xFFU
} can_protocol_command_t;

void card_manager_init(void);
void card_manager_1s_process(void);
void card_manager_process(void);
const card_transaction_multipart_response_t * card_manager_multipart_response_get(uint8_t slot);
void card_manager_multipart_response_clear(uint8_t slot);
uint8_t card_manager_can_receive(uint32_t can_id, uint8_t *can_data, uint8_t can_length);
uint8_t card_manager_identify(uint8_t slot, uint8_t sequence);
uint8_t card_manager_get_relay(uint8_t slot, uint8_t relay);
uint8_t card_manager_set_relay(uint8_t slot, uint8_t sequence, uint8_t relay, uint8_t state);
uint8_t card_manager_get_digital_output(uint8_t slot, uint8_t output);
uint8_t card_manager_set_digital_output(uint8_t slot, uint8_t sequence, uint8_t output, uint8_t state);
uint8_t card_manager_get_pwm(uint8_t slot, uint8_t pwm);
uint8_t card_manager_set_pwm(uint8_t slot, uint8_t sequence, uint8_t pwm, uint8_t duty_cycle);
uint8_t card_manager_uart_send(uint8_t command);
uint8_t card_manager_uart_receive(protocol_uart_response_t *response, uint16_t length);
uint8_t card_manager_i2c_config(uint8_t slot, uint8_t sequence, uint32_t clock_frequency, uint8_t addressing_mode);
uint8_t card_manager_i2c_write(uint8_t slot, uint8_t sequence, uint16_t addr, uint16_t length, const uint8_t *data);
uint8_t card_manager_i2c_read( uint8_t slot, uint8_t sequence, uint16_t addr, uint16_t length);
uint8_t card_manager_i2c_reg_write( uint8_t slot, uint8_t sequence, uint16_t addr, uint8_t reg, uint16_t length, const uint8_t *data);
uint8_t card_manager_i2c_reg_read( uint8_t slot, uint8_t sequence, uint16_t addr, uint8_t reg, uint16_t length);
uint8_t card_manager_spi_config( uint8_t slot, uint8_t sequence, uint32_t frequency, uint8_t cpol, uint8_t cpha, uint8_t bit_order, uint8_t data_size);
uint8_t card_manager_spi_transfer_byte(uint8_t slot, uint8_t sequence, uint8_t tx);
uint8_t card_manager_spi_transfer_word(uint8_t slot, uint8_t sequence, const uint8_t *tx, uint16_t length);
uint8_t card_manager_uart_config(uint8_t slot, uint8_t sequence, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);
uint8_t card_manager_uart_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length);
uint8_t card_manager_uart_read(uint8_t slot, uint8_t sequence);
uint8_t card_manager_rs232_config(uint8_t slot, uint8_t sequence, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);
uint8_t card_manager_rs232_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length);
uint8_t card_manager_rs232_read(uint8_t slot, uint8_t sequence);
uint8_t card_manager_rs485_config(uint8_t slot, uint8_t sequence, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits);
uint8_t card_manager_rs485_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length);
uint8_t card_manager_rs485_read(uint8_t slot, uint8_t sequence);
uint8_t card_manager_can_config(uint8_t slot, uint8_t sequence, uint32_t nominal_bitrate, uint32_t data_bitrate, uint8_t fd_enabled, uint8_t brs_enabled, uint8_t mode, uint8_t can_termination);
uint8_t card_manager_can_std_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length);
uint8_t card_manager_can_fd_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length);
uint8_t card_manager_can_read(uint8_t slot, uint8_t sequence);
uint8_t card_manager_multipart_test(uint8_t slot, uint8_t sequence);
const card_transaction_response_t *card_manager_response_get(uint8_t slot);
void card_manager_response_clear(uint8_t slot);

#endif // CARD_MANAGER_H