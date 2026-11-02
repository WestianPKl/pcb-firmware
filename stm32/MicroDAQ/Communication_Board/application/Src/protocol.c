#include <stdio.h>
#include "protocol.h"
#include "support.h"
#include "input.h"
#include "can.h"
#include "led.h"
#include "version.h"
#include "fault.h"
#include "sync.h"
#include "i2c.h"
#include "spi.h"
#include "uart.h"
#include "rs232.h"
#include "rs485.h"

volatile communication_status_t communication_status = {0};

typedef struct
{
    uint8_t active;
    uint8_t sequence;

} uart_tx_transaction_t;

typedef struct
{
    uint8_t active;
    uint8_t sequence;
    uint8_t fd_enabled;
    uint8_t completed;
    uint8_t result;
} can2_tx_transaction_t;

typedef struct
{
    volatile uint8_t ready;
    uint16_t length;
    uint8_t data[RESPONSE_PROTOCOL_MAX_PAYLOAD];

} uart_rx_mailbox_t;

typedef struct
{
    volatile uint8_t ready;
    uint32_t id;
    can_id_type_t id_type;
    uint8_t fd;
    uint8_t brs;
    uint8_t length;
    uint8_t data[64];
} can2_rx_mailbox_t;

#define MULTIPART_TEST_LENGTH 113U

typedef struct
{
    uint8_t active;
    uint8_t command;
    uint8_t sequence;
    uint16_t length;
    uint16_t count;
    uint16_t package;
    const uint8_t *data;

} multipart_tx_t;

static multipart_tx_t multipart_tx = {0};
static uint8_t multipart_test_data[MULTIPART_TEST_LENGTH];

static uart_tx_transaction_t uart5_tx_transaction = {0};
static uart_rx_mailbox_t uart5_rx_mailbox = {0};
static uart_tx_transaction_t rs232_tx_transaction = {0};
static uart_rx_mailbox_t rs232_rx_mailbox = {0};
static uart_tx_transaction_t rs485_tx_transaction = {0};
static uart_rx_mailbox_t rs485_rx_mailbox = {0};
static can2_tx_transaction_t can2_tx_transaction = {0};
static can2_rx_mailbox_t can2_rx_mailbox = {0};

static uint8_t service_reset_reason = 0U;
static uint32_t service_uptime = 0U;
static uint8_t heartbeat_counter = 0U;

static uint8_t can_fd_pad_length(uint8_t length)
{
    if (length <= 8U)  return length;
    if (length <= 12U) return 12U;
    if (length <= 16U) return 16U;
    if (length <= 20U) return 20U;
    if (length <= 24U) return 24U;
    if (length <= 32U) return 32U;
    if (length <= 48U) return 48U;
    if (length <= 64U) return 64U;

    return 0U;
}

static uint8_t identify_active = 0U;
static uint8_t identify_step = 0U;

static void identify_start(void)
{
    identify_step = 0U;
    identify_active = 1U;

    led_off(GPIOA, LED4_PIN);
}

void protocol_service_init(const reset_reason_t *reason)
{
    heartbeat_counter = 0U;
    service_uptime = 0U;

    if (reason != NULL)
    {
        service_reset_reason = reset_reason_pack(reason);
    }
    else
    {
        service_reset_reason = 0U;
    }
}

static uint8_t service_can_state_get(void)
{
    switch (can_get_state())
    {
        case CAN_STATE_WARNING:
            return SERVICE_CAN_WARNING;

        case CAN_STATE_PASSIVE:
            return SERVICE_CAN_PASSIVE;

        case CAN_STATE_BUS_OFF:
            return SERVICE_CAN_BUS_OFF;

        case CAN_STATE_ACTIVE:
        default:
            return SERVICE_CAN_ACTIVE;
    }
}

static uint8_t service_status_flags_get(void)
{
    uint8_t flags = 0U;

    if (board_info_valid() != 0U)
    {
        flags |= STATUS_READY;
    }

    if (fault_active() != 0U)
    {
        flags |= STATUS_FAULT;
    }

    switch (can_get_state())
    {
        case CAN_STATE_WARNING:
            flags |= STATUS_CAN_WARNING;
            break;

        case CAN_STATE_PASSIVE:
            flags |= STATUS_CAN_PASSIVE;
            break;

        case CAN_STATE_BUS_OFF:
            flags |= STATUS_CAN_BUS_OFF;
            break;

        case CAN_STATE_ACTIVE:
        default:
            break;
    }

    if (sync_counter != 0U)
    {
        flags |= STATUS_SYNC_SEEN;
    }

    if ((service_reset_reason & RESET_REASON_IWDG) != 0U)
    {
        flags |= STATUS_WATCHDOG_RESET;
    }

    return flags;
}

static uint8_t multipart_tx_start(uint8_t command, uint8_t sequence, const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U) || (multipart_tx.active != 0U))
    {
        return 0U;
    }

    multipart_tx.command = command;
    multipart_tx.sequence = sequence;
    multipart_tx.data = data;
    multipart_tx.length = length;
    multipart_tx.count = (uint16_t)(((uint32_t)length + MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD - 1U) / MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD);
    multipart_tx.package = 1U;
    multipart_tx.active = 1U;

    return 1U;
}

uint8_t can_protocol_process_request(const can_protocol_request_t *request, can_protocol_response_t *frame)
{
    comm_led_activity();
    uint8_t length = 0U;
    uint8_t status;

    frame->command = request->command;
    frame->sequence = request->sequence;

    switch (request->command) {
        case CMD_PING:
        {
            frame->data[0] = 0x11;
            frame->data[1] = 0x22;
            frame->data[2] = 0x33;
            length = 3U;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_IDENTIFY:
        {
            identify_start();
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_GET_DEVICE_INFO:
        {
            const board_info_t *info = board_info_get();
            if (board_info_valid())
            {
                fault_clear(FAULT_BOARD_INFO);

                frame->data[0] = (uint8_t)(info->hw_major);
                frame->data[1] = (uint8_t)(info->hw_minor);
                frame->data[2] = (uint8_t)(info->year >> 8);
                frame->data[3] = (uint8_t)(info->year);
                frame->data[4] = (uint8_t)(info->month);
                frame->data[5] = (uint8_t)(info->day); 
                frame->data[6] = (uint8_t)(info->serial_number >> 24);
                frame->data[7] = (uint8_t)(info->serial_number >> 16);
                frame->data[8] = (uint8_t)(info->serial_number >> 8);
                frame->data[9] = (uint8_t)(info->serial_number);
                frame->data[10] = (uint8_t)(info->card_type >> 8);
                frame->data[11] = (uint8_t)(info->card_type);
                frame->data[12] = (uint8_t)(fw_version.major);
                frame->data[13] = (uint8_t)(fw_version.minor);
                frame->data[14] = (uint8_t)(fw_version.patch);
                frame->data[15] = (uint8_t)(fw_version.year >> 8);
                frame->data[16] = (uint8_t)(fw_version.year);
                frame->data[17] = (uint8_t)(fw_version.month);
                frame->data[18] = (uint8_t)(fw_version.day);
                frame->data[19] = (uint8_t)(info->capabilities.flags >> 24);
                frame->data[20] = (uint8_t)(info->capabilities.flags >> 16);
                frame->data[21] = (uint8_t)(info->capabilities.flags >> 8);
                frame->data[22] = (uint8_t)(info->capabilities.flags);
                frame->data[23] = (uint8_t)(info->capabilities.relay_count); 
                frame->data[24] = (uint8_t)(info->capabilities.voltage_count);
                frame->data[25] = (uint8_t)(info->capabilities.current_count);
                frame->data[26] = (uint8_t)(info->capabilities.frequency_count);
                frame->data[27] = (uint8_t)(info->capabilities.digital_input_count);
                frame->data[28] = (uint8_t)(info->capabilities.digital_output_count);
                frame->data[29] = (uint8_t)(info->capabilities.pwm_count);
                frame->data[30] = (uint8_t)(info->capabilities.analog_output_count);
                frame->data[31] = (uint8_t)(info->capabilities.can_count);
                frame->data[32] = (uint8_t)(info->capabilities.lin_count);
                frame->data[33] = (uint8_t)(info->capabilities.uart_count);
                frame->data[34] = (uint8_t)(info->capabilities.rs485_count);
                frame->data[35] = (uint8_t)(info->capabilities.rs232_count);
                frame->data[36] = (uint8_t)(info->capabilities.i2c_count);
                frame->data[37] = (uint8_t)(info->capabilities.spi_count);
                frame->data[38] = (uint8_t)(info->slot_count);
                length = 39U;
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                fault_set(FAULT_BOARD_INFO);
                status = PROTOCOL_STATUS_ERROR;
            }
            break;
        }
        case CMD_I2C_CONFIG:
        {
            i2c_config_t config;

            if (request->length != 5U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            config.clock_frequency = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);
            config.addressing_mode = request->data[4];

            if (i2c1_config(&config) == 1U)
            {
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                status = PROTOCOL_STATUS_ERROR;
            }

            length = 0U;
            break;
        }
        case CMD_I2C_WRITE:
        {
            if (request->length < 4U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint16_t addr = ((uint16_t)request->data[0] << 8U) | ((uint16_t)request->data[1]);
            uint16_t len = ((uint16_t)request->data[2] << 8U) | ((uint16_t)request->data[3]);

            if (len == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if ((uint16_t)request->length != (4U + len))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.error_flags &= ~COMM_I2C_ERROR;
            communication_status.interface_status |= COMM_I2C_BUSY;

            if (i2c1_write(addr, &request->data[4], len) != 1U)
            {
                communication_status.error_flags |= COMM_I2C_ERROR;
                communication_status.interface_status &= ~COMM_I2C_BUSY;

                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.interface_status &= ~COMM_I2C_BUSY;

            length = 0U;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_I2C_READ:
        {
            if (request->length != 4U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint16_t addr = ((uint16_t)request->data[0] << 8U) | ((uint16_t)request->data[1]);
            uint16_t len = ((uint16_t)request->data[2] << 8U) | ((uint16_t)request->data[3]);
            if ((len == 0U) || (len > RESPONSE_PROTOCOL_MAX_PAYLOAD))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.error_flags &= ~COMM_I2C_ERROR;
            communication_status.interface_status |= COMM_I2C_BUSY;

            if (i2c1_read(addr, frame->data, len) != 1U)
            {
                communication_status.error_flags |= COMM_I2C_ERROR;
                communication_status.interface_status &= ~COMM_I2C_BUSY;
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.interface_status &= ~COMM_I2C_BUSY;

            length = (uint8_t)len;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_I2C_REG_WRITE:
        {
            if (request->length < 5U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint16_t addr = ((uint16_t)request->data[0] << 8U) | ((uint16_t)request->data[1]);
            uint8_t reg = request->data[2];
            uint16_t len = ((uint16_t)request->data[3] << 8U) | ((uint16_t)request->data[4]);
            if ((len == 0U) || (request->length != (uint8_t)(5U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.error_flags &= ~COMM_I2C_ERROR;
            communication_status.interface_status |= COMM_I2C_BUSY;

            if (i2c1_write_reg(addr, reg, &request->data[5], len) != 1U)
            {
                communication_status.error_flags |= COMM_I2C_ERROR;
                communication_status.interface_status &= ~COMM_I2C_BUSY;
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.interface_status &= ~COMM_I2C_BUSY;

            length = 0U;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_I2C_REG_READ:
        {
            if (request->length != 5U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint16_t addr = ((uint16_t)request->data[0] << 8U) | (uint16_t)request->data[1];
            uint8_t reg = request->data[2];
            uint16_t len = ((uint16_t)request->data[3] << 8U) | (uint16_t)request->data[4];

            if ((len == 0U) || (len > RESPONSE_PROTOCOL_MAX_PAYLOAD))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.error_flags &= ~COMM_I2C_ERROR;
            communication_status.interface_status |= COMM_I2C_BUSY;

            if (i2c1_read_reg(addr, reg, frame->data, len) != 1U)
            {
                communication_status.error_flags |= COMM_I2C_ERROR;
                communication_status.interface_status &= ~COMM_I2C_BUSY;

                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.interface_status &= ~COMM_I2C_BUSY;

            length = (uint8_t)len;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_SPI_CONFIG:
        {
            spi_config_t config;
            if (request->length != 8U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            config.frequency = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);
            config.cpol = request->data[4];
            config.cpha = request->data[5];
            config.bit_order = request->data[6];
            config.data_size = request->data[7];

            if (spi1_config(&config) == 1U)
            {
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                status = PROTOCOL_STATUS_ERROR;
            }

            length = 0U;
            break;
        }
        case CMD_SPI_TRANSFER_BYTE:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t tx = request->data[0];
            uint8_t rx;

            communication_status.error_flags &= ~COMM_SPI_ERROR;
            communication_status.interface_status |= COMM_SPI_BUSY;

            if (spi1_transfer_byte(tx, &rx) != 1U)
            {
                communication_status.error_flags |= COMM_SPI_ERROR;
                communication_status.interface_status &= ~COMM_SPI_BUSY;
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.interface_status &= ~COMM_SPI_BUSY;

            frame->data[0] = rx;

            length = 1U;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_SPI_TRANSFER_WORD:
        {
            if (request->length < 2U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint16_t len = ((uint16_t)request->data[0] << 8U) | (uint16_t)request->data[1];

            if ((len == 0U) || (len > RESPONSE_PROTOCOL_MAX_PAYLOAD) || (request->length != (uint8_t)(2U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.error_flags &= ~COMM_SPI_ERROR;
            communication_status.interface_status |= COMM_SPI_BUSY;

            if (spi1_transfer(&request->data[2], frame->data, len) != 1U)
            {
                communication_status.error_flags |= COMM_SPI_ERROR;
                communication_status.interface_status &= ~COMM_SPI_BUSY;

                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            communication_status.interface_status &= ~COMM_SPI_BUSY;

            length = (uint8_t)len;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case CMD_UART_CONFIG:
        {
            serial_config_t config;
            if (request->length != 7U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            config.baudrate = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);
            config.data_bits = request->data[4];
            config.parity = request->data[5];
            config.stop_bits = request->data[6];

            if (uart5_config(&config) == 1U)
            {
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                status = PROTOCOL_STATUS_ERROR;
            }

            length = 0U;
            break;
        }
        case CMD_UART_WRITE:
        {
            if ((request->length == 0U) || (request->length > UART5_TX_BUFFER_SIZE))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (uart5_tx_transaction.active != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uart5_tx_transaction.active = 1U;
            uart5_tx_transaction.sequence = request->sequence;

            if (uart5_dma_write(request->data, request->length) == 0U)
            {
                uart5_tx_transaction = (uart_tx_transaction_t){0};
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return 0U;
        }
        case CMD_UART_READ:
        {
            if (request->length != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (uart5_rx_mailbox.ready == 0U)
            {
                length = 0U;
                status = PROTOCOL_STATUS_OK;
                break;
            }

            uint16_t rx_length = uart5_rx_mailbox.length;

            if (rx_length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
            {
                rx_length = RESPONSE_PROTOCOL_MAX_PAYLOAD;
            }

            for (uint16_t i = 0U; i < rx_length; i++)
            {
                frame->data[i] = uart5_rx_mailbox.data[i];
            }

            length = (uint8_t)rx_length;
            status = PROTOCOL_STATUS_OK;

            uart5_rx_mailbox.ready = 0U;
            uart5_rx_mailbox.length = 0U;

            break;
        }
        case CMD_RS232_CONFIG:
        {
            serial_config_t config;
            if (request->length != 7U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            config.baudrate = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);
            config.data_bits = request->data[4];
            config.parity = request->data[5];
            config.stop_bits = request->data[6];

            if (rs232_config(&config) == 1U)
            {
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                status = PROTOCOL_STATUS_ERROR;
            }

            length = 0U;
            break;
        }
        case CMD_RS232_WRITE:
        {
            if ((request->length == 0U) || (request->length > RS232_TX_BUFFER_SIZE))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (rs232_tx_transaction.active != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            rs232_tx_transaction.active = 1U;
            rs232_tx_transaction.sequence = request->sequence;

            if (rs232_dma_write(request->data, request->length) == 0U)
            {
                rs232_tx_transaction = (uart_tx_transaction_t){0};
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return 0U;
        }
        case CMD_RS232_READ:
        {
            if (request->length != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (rs232_rx_mailbox.ready == 0U)
            {
                length = 0U;
                status = PROTOCOL_STATUS_OK;
                break;
            }

            uint16_t rx_length = rs232_rx_mailbox.length;

            if (rx_length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
            {
                rx_length = RESPONSE_PROTOCOL_MAX_PAYLOAD;
            }

            for (uint16_t i = 0U; i < rx_length; i++)
            {
                frame->data[i] = rs232_rx_mailbox.data[i];
            }

            length = (uint8_t)rx_length;
            status = PROTOCOL_STATUS_OK;

            rs232_rx_mailbox.ready = 0U;
            rs232_rx_mailbox.length = 0U;

            break;
        }
        case CMD_RS485_CONFIG:
        {
            serial_config_t config;
            if (request->length != 7U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            config.baudrate = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);
            config.data_bits = request->data[4];
            config.parity = request->data[5];
            config.stop_bits = request->data[6];

            if (rs485_config(&config) == 1U)
            {
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                status = PROTOCOL_STATUS_ERROR;
            }

            length = 0U;
            break;
        }
        case CMD_RS485_WRITE:
        {
            if ((request->length == 0U) || (request->length > RS485_TX_BUFFER_SIZE))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (rs485_tx_transaction.active != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            rs485_tx_transaction.active = 1U;
            rs485_tx_transaction.sequence = request->sequence;

            if (rs485_dma_write(request->data, request->length) == 0U)
            {
                rs485_tx_transaction = (uart_tx_transaction_t){0};
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return 0U;
        }
        case CMD_RS485_READ:
        {
            if (request->length != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (rs485_rx_mailbox.ready == 0U)
            {
                length = 0U;
                status = PROTOCOL_STATUS_OK;
                break;
            }

            uint16_t rx_length = rs485_rx_mailbox.length;

            if (rx_length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
            {
                rx_length = RESPONSE_PROTOCOL_MAX_PAYLOAD;
            }

            for (uint16_t i = 0U; i < rx_length; i++)
            {
                frame->data[i] = rs485_rx_mailbox.data[i];
            }

            length = (uint8_t)rx_length;
            status = PROTOCOL_STATUS_OK;

            rs485_rx_mailbox.ready = 0U;
            rs485_rx_mailbox.length = 0U;

            break;
        }
        case CMD_CAN_CONFIG:
        {
            can_config_t config;
            if (request->length != 12U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            config.nominal_bitrate = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);
            config.data_bitrate = ((uint32_t)request->data[4] << 24U) | ((uint32_t)request->data[5] << 16U) | ((uint32_t)request->data[6] << 8U) | ((uint32_t)request->data[7]);
            config.fd_enabled = request->data[8];
            config.brs_enabled = request->data[9];
            config.mode = request->data[10];
            config.can_termination = request->data[11];

            if (can2_tx_transaction.active != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (can2_config(&config) == 1U)
            {
                status = PROTOCOL_STATUS_OK;
            }
            else
            {
                status = PROTOCOL_STATUS_ERROR;
            }

            length = 0U;
            break;
        }
        case CMD_CAN_SEND_STD:
        {
            if ((request->length < 4U) || (request->length > 12U))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (can2_tx_transaction.active != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            can2_tx_transaction.active = 1U;
            can2_tx_transaction.sequence = request->sequence;
            can2_tx_transaction.fd_enabled = 0U;
            
            uint32_t std_id = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);

            if (can2_send_std(std_id, &request->data[4], request->length - 4) == 0U)
            {
                can2_tx_transaction = (can2_tx_transaction_t){0};
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return 0U;
        }
        case CMD_CAN_SEND_FD:
        {
            if ((request->length < 4U) || (request->length > 52U))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (can2_tx_transaction.active != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            can2_tx_transaction.active = 1U;
            can2_tx_transaction.sequence = request->sequence;
            can2_tx_transaction.fd_enabled = 1U;

            uint32_t std_id = ((uint32_t)request->data[0] << 24U) | ((uint32_t)request->data[1] << 16U) | ((uint32_t)request->data[2] << 8U) | ((uint32_t)request->data[3]);

            if (can2_send_fd(std_id, &request->data[4], request->length - 4) == 0U)
            {
                can2_tx_transaction = (can2_tx_transaction_t){0};
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return 0U;
        }
        case CMD_CAN_RECEIVE:
        {
            if (request->length != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (can2_rx_mailbox.ready == 0U)
            {
                length = 0U;
                status = PROTOCOL_STATUS_OK;
                break;
            }

            uint8_t rx_length = can2_rx_mailbox.length;

            if (rx_length > 48U)
            {
                can2_rx_mailbox.ready = 0U;
                can2_rx_mailbox.length = 0U;

                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint32_t id = can2_rx_mailbox.id;

            frame->data[0] = (uint8_t)(id >> 24U);
            frame->data[1] = (uint8_t)(id >> 16U);
            frame->data[2] = (uint8_t)(id >> 8U);
            frame->data[3] = (uint8_t)id;
            frame->data[4] = ((can2_rx_mailbox.id_type == CAN_ID_EXT) ? 0x01U : 0U) | ((can2_rx_mailbox.fd != 0U) ? 0x02U : 0U) | ((can2_rx_mailbox.brs != 0U) ? 0x04U : 0U);
            frame->data[5] = rx_length;

            for (uint8_t i = 0U; i < rx_length; i++)
            {
                frame->data[6U + i] = can2_rx_mailbox.data[i];
            }

            length = (uint8_t)(6U + rx_length);
            status = PROTOCOL_STATUS_OK;

            can2_rx_mailbox.ready = 0U;
            can2_rx_mailbox.length = 0U;

            break;
        }
        case CMD_MULTIPART_TEST:
        {
            if (request->length != 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (multipart_tx.active != 0U)
            {
                status = PROTOCOL_STATUS_BUSY;
                break;
            }

            for (uint16_t i = 0U; i < MULTIPART_TEST_LENGTH; i++)
            {
                multipart_test_data[i] = (uint8_t)(0x11U + i);
            }

            if (multipart_tx_start(request->command, request->sequence, multipart_test_data, MULTIPART_TEST_LENGTH) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return 0U;
        }
        default:
        {
            status = PROTOCOL_STATUS_ERROR;
            break;
        }
    }

    frame->length = length;
    frame->status = status;

    return (uint8_t)(4U + length);
}

void multipart_tx_process(void)
{
    if (multipart_tx.active == 0U)
    {
        return;
    }

    uint32_t offset = ((uint32_t)multipart_tx.package - 1U) * MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD;
    uint16_t remaining = (uint16_t)((uint32_t)multipart_tx.length - offset);
    uint8_t chunk_length = (remaining > MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD) ? MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD : (uint8_t)remaining;

    uint8_t frame[64] = {0};

    frame[0] = multipart_tx.command;
    frame[1] = multipart_tx.sequence;
    frame[2] = PROTOCOL_STATUS_MULTIPART;
    frame[3] = chunk_length;
    frame[4] = (uint8_t)(multipart_tx.count >> 8U);
    frame[5] = (uint8_t)multipart_tx.count;
    frame[6] = (uint8_t)(multipart_tx.package >> 8U);
    frame[7] = (uint8_t)multipart_tx.package;

    for (uint8_t i = 0U; i < chunk_length; i++)
    {
        frame[8U + i] = multipart_tx.data[offset + i];
    }

    uint8_t fd_length = can_fd_pad_length((uint8_t)(8U + chunk_length));

    if (can_send_fd(can_id_make(slot_id, 0x01U), frame, fd_length) == 0U)
    {
        return;
    }

    if (multipart_tx.package >= multipart_tx.count)
    {
        multipart_tx = (multipart_tx_t){0};
    }
    else
    {
        multipart_tx.package++;
    }
}

void can_process_response(const can_protocol_request_t *request, uint8_t can_length)
{
    can_protocol_response_t response = {0};

    if (can_length < 3U)
    {
        return;
    }

    if (request->length > REQUEST_PROTOCOL_MAX_PAYLOAD)
    {
        return;
    }

    uint8_t protocol_length = (uint8_t)(3U + request->length);

    if (protocol_length > can_length)
    {
        return;
    }

    uint8_t response_length = can_protocol_process_request(request, &response);
    uint8_t fd_length = can_fd_pad_length(response_length);

    if (fd_length != 0U)
    {
        can_send_fd(can_id_make(slot_id, 0x01U), (const uint8_t *)&response, fd_length);
    }
}

static uint8_t communication_send_response(
    const can_protocol_response_t *response)
{
    if (response == NULL)
    {
        return 0U;
    }

    uint8_t protocol_length = (uint8_t)(4U + response->length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);

    if (fd_length == 0U)
    {
        return 0U;
    }

    return can_send_fd(can_id_make(slot_id, 0x01U), (const uint8_t *)response, fd_length);
}

// Service Cyclic Frames
static uint8_t send_frame_0xX10(void)
{
    service_status_t status = {0};

    status.heartbeat_counter = heartbeat_counter;
    status.status_flags = service_status_flags_get();
    status.fault_flags = fault_get();
    status.reset_reason = service_reset_reason;
    status.can_state = service_can_state_get();
    status.uptime = service_uptime;
    status.protocol_major = PROTOCOL_VERSION_MAJOR;
    status.protocol_minor = PROTOCOL_VERSION_MINOR;

    uint8_t data_can[12] = {0};

    data_can[0]  = status.heartbeat_counter;
    data_can[1]  = status.status_flags;
    data_can[2]  = (uint8_t)(status.fault_flags >> 8U);
    data_can[3]  = (uint8_t)status.fault_flags;
    data_can[4]  = status.reset_reason;
    data_can[5]  = status.can_state;
    data_can[6]  = (uint8_t)(status.uptime >> 24U);
    data_can[7]  = (uint8_t)(status.uptime >> 16U);
    data_can[8]  = (uint8_t)(status.uptime >> 8U);
    data_can[9]  = (uint8_t)status.uptime;
    data_can[10] = status.protocol_major;
    data_can[11] = status.protocol_minor;

    heartbeat_counter++;

    return can_send_fd(can_id_make(slot_id, 0x10U), data_can, sizeof(data_can));
}
// END Service Cyclic Frames

// Communication Board Cyclic Frames
static uint8_t send_frame_0xX20(void)
{

    uint8_t data_can[2];

    data_can[0] = 0U;
    data_can[1] = 0U;

    return can_send_fd(can_id_make(slot_id, 0x20U), data_can, sizeof(data_can));
}
// END Communication Board Cyclic Frames

void can_send_100ms_frames(void)
{
    static uint8_t slot = 0U;
    uint8_t sent = 0U;

    switch (slot)
    {
        case 0:
            sent = send_frame_0xX20();
            break;
        default:
            slot = 0U;
            break;
    }

    if (sent != 0U)
    {
        slot++;

        if (slot >= 1U)
        {
            slot = 0U;
        }
    }
}

void can_send_1s_frames(void)
{
    static uint8_t slot = 0U;
    uint8_t sent = 0U;

    service_uptime++;

    switch (slot)
    {
        case 0:
            sent = send_frame_0xX10();
            break;
        default:
            slot = 0U;
            break;
    }

    if (sent != 0U)
    {
        slot++;

        if (slot >= 1U)
        {
            slot = 0U;
        }
    }
}

void identify_process(void)
{
    led_off(GPIOA, LED4_PIN);

    if (identify_step >= 12U)
    {
        identify_active = 0U;
        identify_step = 0U;
        return;
    }

    if ((identify_step & 0x01U) == 0U)
    {
        led_on(GPIOA, LED4_PIN);
    }

    identify_step++;
}

void uart5_tx_callback(uint8_t tx_status)
{
    if (uart5_tx_transaction.active == 0U)
    {
        return;
    }

    can_protocol_response_t response = {0};

    response.command = CMD_UART_WRITE;
    response.sequence = uart5_tx_transaction.sequence;
    response.status = (tx_status != 0U) ? PROTOCOL_STATUS_OK : PROTOCOL_STATUS_ERROR;
    response.length = 0U;

    if (communication_send_response(&response) != 0U)
    {
        uart5_tx_transaction = (uart_tx_transaction_t){0};
    }

    uart5_tx_transaction = (uart_tx_transaction_t){0};
}

void uart5_rx_callback(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    if (uart5_rx_mailbox.ready != 0U)
    {
        return;
    }

    if (length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
    {
        length = RESPONSE_PROTOCOL_MAX_PAYLOAD;
    }

    uart5_rx_mailbox.length = length;

    for (uint16_t i = 0U; i < length; i++)
    {
        uart5_rx_mailbox.data[i] = data[i];
    }

    uart5_rx_mailbox.ready = 1U;
}

void rs232_tx_callback(uint8_t tx_status)
{
    if (rs232_tx_transaction.active == 0U)
    {
        return;
    }

    can_protocol_response_t response = {0};

    response.command = CMD_RS232_WRITE;
    response.sequence = rs232_tx_transaction.sequence;
    response.status = (tx_status != 0U) ? PROTOCOL_STATUS_OK : PROTOCOL_STATUS_ERROR;
    response.length = 0U;

    if (communication_send_response(&response) != 0U)
    {
        rs232_tx_transaction = (uart_tx_transaction_t){0};
    }

    rs232_tx_transaction = (uart_tx_transaction_t){0};
}

void rs232_rx_callback(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    if (rs232_rx_mailbox.ready != 0U)
    {
        return;
    }

    if (length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
    {
        length = RESPONSE_PROTOCOL_MAX_PAYLOAD;
    }

    rs232_rx_mailbox.length = length;

    for (uint16_t i = 0U; i < length; i++)
    {
        rs232_rx_mailbox.data[i] = data[i];
    }

    rs232_rx_mailbox.ready = 1U;
}

void rs485_tx_callback(uint8_t tx_status)
{
    if (rs485_tx_transaction.active == 0U)
    {
        return;
    }

    can_protocol_response_t response = {0};

    response.command = CMD_RS485_WRITE;
    response.sequence = rs485_tx_transaction.sequence;
    response.status = (tx_status != 0U) ? PROTOCOL_STATUS_OK : PROTOCOL_STATUS_ERROR;
    response.length = 0U;

    if (communication_send_response(&response) != 0U)
    {
        rs485_tx_transaction = (uart_tx_transaction_t){0};
    }

    rs485_tx_transaction = (uart_tx_transaction_t){0};
}

void rs485_rx_callback(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    if (rs485_rx_mailbox.ready != 0U)
    {
        return;
    }

    if (length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
    {
        length = RESPONSE_PROTOCOL_MAX_PAYLOAD;
    }

    rs485_rx_mailbox.length = length;

    for (uint16_t i = 0U; i < length; i++)
    {
        rs485_rx_mailbox.data[i] = data[i];
    }

    rs485_rx_mailbox.ready = 1U;
}


void can2_tx_callback(uint8_t tx_status)
{
    if (can2_tx_transaction.active == 0U)
    {
        return;
    }

    can2_tx_transaction.result = tx_status;
    can2_tx_transaction.completed = 1U;
}

void can2_rx_callback(uint32_t id, can_id_type_t id_type, const uint8_t *data, uint8_t length, uint8_t fd, uint8_t brs)
{
    if ((data == NULL) || (length > 64U) || (fd > 1U) || (brs > 1U))
    {
        return;
    }

    if (can2_rx_mailbox.ready != 0U)
    {
        return;
    }

    can2_rx_mailbox.id = id;
    can2_rx_mailbox.id_type = id_type;
    can2_rx_mailbox.fd = fd;
    can2_rx_mailbox.brs = brs;
    can2_rx_mailbox.length = length;

    for (uint8_t i = 0U; i < length; i++)
    {
        can2_rx_mailbox.data[i] = data[i];
    }

    can2_rx_mailbox.ready = 1U;
}

void can2_tx_process(void)
{
    if ((can2_tx_transaction.active == 0U) || (can2_tx_transaction.completed == 0U))
    {
        return;
    }

    can_protocol_response_t response = {0};

    response.command = (can2_tx_transaction.fd_enabled != 0U) ? CMD_CAN_SEND_FD : CMD_CAN_SEND_STD;

    response.sequence = can2_tx_transaction.sequence;
    response.status = (can2_tx_transaction.result != 0U) ? PROTOCOL_STATUS_OK : PROTOCOL_STATUS_ERROR;
    response.length = 0U;

    if (communication_send_response(&response) != 0U)
    {
        can2_tx_transaction = (can2_tx_transaction_t){0};
    }
}