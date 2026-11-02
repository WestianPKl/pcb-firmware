#include "protocol.h"
#include "controller_tcp.h"
#include "card_manager.h"
#include "support.h"
#include "can.h"
#include "timer.h"
#include "fault.h"

static uint8_t rx_frame[PROTOCOL_REQUEST_MAX];
static uint16_t rx_index = 0U;
static uint16_t rx_payload_length = 0U;
static uint16_t rx_expected_length = 0U;

static uint8_t async_response_frame[PROTOCOL_RESPONSE_MAX];
static uint8_t response_frame[PROTOCOL_RESPONSE_MAX];

static uint8_t service_reset_reason = 0U;
volatile uint32_t service_uptime = 0U;

static void protocol_rx_reset(void)
{
    rx_index = 0U;
    rx_payload_length = 0U;
    rx_expected_length = 0U;
}

static uint8_t protocol_command_from_can(uint8_t command)
{
    switch (command)
    {
        case CAN_PROTOCOL_CMD_IDENTIFY_CARD:
            return PROTOCOL_CMD_IDENTIFY_CARD;
        case CAN_PROTOCOL_CMD_SET_RELAY:
            return PROTOCOL_CMD_SET_RELAY;
        case CAN_PROTOCOL_CMD_SET_DIGITAL_OUTPUT:
            return PROTOCOL_CMD_SET_DIGITAL_OUTPUT;
        case CAN_PROTOCOL_CMD_SET_PWM:
            return PROTOCOL_CMD_SET_PWM;
        default:
            break;
    }

    if ((command >= CAN_PROTOCOL_CMD_I2C_CONFIG) && (command <= CAN_PROTOCOL_CMD_CAN_RECEIVE))
    {
        return (uint8_t)(command + 0x10U);
    }

    return command;
}

void protocol_service_init(const reset_reason_t *reason)
{
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

void protocol_process(void)
{
    for (uint8_t slot = 1U; slot <= slot_count; slot++)
    {
        const card_transaction_response_t *response = card_manager_response_get(slot);

        if (response == NULL)
        {
            continue;
        }

        uint16_t length = 0U;
        uint8_t command = protocol_command_from_can(response->command);
        uint8_t status = (response->status == CAN_PROTOCOL_STATUS_OK) ? PROTOCOL_STATUS_OK : PROTOCOL_STATUS_ERROR;

        async_response_frame[0] = PROTOCOL_MAGIC;
        async_response_frame[3] = response->sequence;
        async_response_frame[4] = command;

        if (status == PROTOCOL_STATUS_OK)
        {
            switch (command)
            {
                case PROTOCOL_CMD_IDENTIFY_CARD:
                {
                    if (response->length != 0U)
                    {
                        status = PROTOCOL_STATUS_ERROR;
                    }
                    break;
                }

                case PROTOCOL_CMD_SET_RELAY:
                case PROTOCOL_CMD_SET_DIGITAL_OUTPUT:
                case PROTOCOL_CMD_SET_PWM:
                {
                    if (response->length != 2U)
                    {
                        status = PROTOCOL_STATUS_ERROR;
                        break;
                    }

                    async_response_frame[6] = slot;
                    async_response_frame[7] = response->data[0];
                    async_response_frame[8] = response->data[1];

                    length = 3U;
                    break;
                }

                default:
                {
                    length = response->length;

                    for (uint16_t i = 0U; i < length; i++)
                    {
                        async_response_frame[6U + i] = response->data[i];
                    }

                    break;
                }
            }
        }

        async_response_frame[1] = (uint8_t)(length >> 8U);
        async_response_frame[2] = (uint8_t)length;
        async_response_frame[5] = status;

        uint16_t crc = crc16_ccitt(&async_response_frame[1], (uint16_t)(5U + length));
        async_response_frame[6U + length] = (uint8_t)(crc >> 8U);
        async_response_frame[7U + length] = (uint8_t)crc;

        if (controller_tcp_send(async_response_frame, (uint16_t)(8U + length)) == 0U)
        {
            return;
        }

        card_manager_response_clear(slot);
        return;
    }

    for (uint8_t slot = 1U; slot <= slot_count; slot++)
    {
        const card_transaction_multipart_response_t *response = card_manager_multipart_response_get(slot);

        if (response == NULL)
        {
            continue;
        }

        uint16_t length = 0U;
        uint8_t status = PROTOCOL_STATUS_ERROR;

        if ((response->status == CAN_PROTOCOL_STATUS_OK) && (response->length <= PROTOCOL_MAX_PAYLOAD))
        {
            length = response->length;
            status = PROTOCOL_STATUS_OK;

            for (uint16_t i = 0U; i < length; i++)
            {
                async_response_frame[6U + i] = response->data[i];
            }
        }

        async_response_frame[0] = PROTOCOL_MAGIC;
        async_response_frame[1] = (uint8_t)(length >> 8U);
        async_response_frame[2] = (uint8_t)length;
        async_response_frame[3] = response->sequence;
        async_response_frame[4] = protocol_command_from_can(response->command);
        async_response_frame[5] = status;

        uint16_t crc = crc16_ccitt(&async_response_frame[1], (uint16_t)(5U + length));
        async_response_frame[6U + length] = (uint8_t)(crc >> 8U);
        async_response_frame[7U + length] = (uint8_t)crc;

        if (controller_tcp_send(async_response_frame, (uint16_t)(8U + length)) == 0U)
        {
            return;
        }
        card_manager_multipart_response_clear(slot);
        return;
    }
}

uint16_t protocol_process_request(const protocol_request_t *request, uint8_t *frame)
{
    uint16_t length = 0U;
    protocol_status_t status = PROTOCOL_STATUS_ERROR;

    frame[0] = PROTOCOL_MAGIC;

    frame[3] = request->sequence;
    frame[4] = request->command;

    switch (request->command)
    {
        case PROTOCOL_CMD_PING:
        {
            if (request->length == 0U)
            {
                frame[6] = 0x11U;
                frame[7] = 0x22U;
                frame[8] = 0x33U;

                length = 3U;
                status = PROTOCOL_STATUS_OK;
            }

            break;
        }
        case PROTOCOL_CMD_GET_CONTROLLER_INFO:
        {
            if (request->length == 0U)
            {
                const board_info_t *info = board_info_get();
                if (board_info_valid())
                {
                    frame[6] = (uint8_t)(info->card_type >> 8);
                    frame[7] = (uint8_t)(info->card_type);
                    frame[8] = (uint8_t)(info->hw_major);
                    frame[9] = (uint8_t)(info->hw_minor);
                    frame[10] = (uint8_t)(info->year >> 8U);
                    frame[11] = (uint8_t)(info->year);
                    frame[12] = (uint8_t)(info->month);
                    frame[13] = (uint8_t)(info->day);
                    frame[14] = (uint8_t)(info->serial_number >> 24U);
                    frame[15] = (uint8_t)(info->serial_number >> 16U);
                    frame[16] = (uint8_t)(info->serial_number >> 8U);
                    frame[17] = (uint8_t)(info->serial_number);
                    frame[18] = (uint8_t)(fw_version.major);
                    frame[19] = (uint8_t)(fw_version.minor);
                    frame[20] = (uint8_t)(fw_version.patch);
                    frame[21] = (uint8_t)(fw_version.year >> 8U);
                    frame[22] = (uint8_t)(fw_version.year);
                    frame[23] = (uint8_t)(fw_version.month);
                    frame[24] = (uint8_t)(fw_version.day);
                    frame[25] = (uint8_t)(PROTOCOL_VERSION_MAJOR);
                    frame[26] = (uint8_t)(PROTOCOL_VERSION_MINOR);
                    frame[27] = (uint8_t)(service_uptime >> 24U);
                    frame[28] = (uint8_t)(service_uptime >> 16U);
                    frame[29] = (uint8_t)(service_uptime >> 8U);
                    frame[30] = (uint8_t)(service_uptime);
                    frame[31] = (uint8_t)(info->slot_count);
                    length = 26U;
                    status = PROTOCOL_STATUS_OK;
                } 
                else
                {
                    status = PROTOCOL_STATUS_ERROR;
                }
            }
            break;
        }
        case PROTOCOL_CMD_GET_CARDS_INFO:
        {
            if (request->length == 0U)
            {
                uint16_t slot_offset = 6U;
                for (uint8_t i = 0U; i < slot_count; i++)
                {
                    frame[slot_offset++] = (uint8_t)(card_slots[i].detected);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fault);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].identified);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.card_type >> 8);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.card_type);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.hw_major);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.hw_minor);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.year >> 8U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.year);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.month);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.day);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.serial_number >> 24U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.serial_number >> 16U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.serial_number >> 8U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.serial_number);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.major);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.minor);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.patch);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.year >> 8U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.year);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.month);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].fw_version.day);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].service.protocol_major);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].service.protocol_minor);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].service.uptime >> 24U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].service.uptime >> 16U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].service.uptime >> 8U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].service.uptime);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.flags >> 24U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.flags >> 16U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.flags >> 8U);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.flags);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.relay_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.voltage_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.current_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.frequency_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.digital_input_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.digital_output_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.pwm_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.analog_output_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.can_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.lin_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.uart_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.rs485_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.rs232_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.i2c_count);
                    frame[slot_offset++] = (uint8_t)(card_slots[i].board_info.capabilities.spi_count);
                }

                length = (uint16_t)(slot_offset - 6U);
                status = PROTOCOL_STATUS_OK;
            }
            break;
        }
        case PROTOCOL_CMD_IDENTIFY_CARD:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];

            if ((slot == 0U) || (slot > slot_count))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot_index = (uint8_t)(slot - 1U);

            if (card_slots[slot_index].detected == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].identified == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_identify(slot, request->sequence) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_GET_RELAY:
        {
            if (request->length != 2U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t relay = request->data[1];

            if ((slot == 0U) || (slot > slot_count))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot_index = (uint8_t)(slot - 1U);

            if (card_slots[slot_index].detected == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].identified == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].service.online == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if ((card_slots[slot_index].board_info.capabilities.flags & CAP_RELAY) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (relay >= card_slots[slot_index].board_info.capabilities.relay_count)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            frame[6] = card_manager_get_relay(slot, relay);

            length = 1U;
            status = PROTOCOL_STATUS_OK;

            break;
        }
        case PROTOCOL_CMD_SET_RELAY:
        {
            if (request->length != 3U)
            {
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t relay = request->data[1];
            uint8_t state = request->data[2];

            if (card_manager_set_relay(slot, request->sequence, relay, state) == 0U)
            {
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_GET_DIGITAL_OUTPUT:
        {
            if (request->length != 2U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t output = request->data[1];

            if ((slot == 0U) || (slot > slot_count))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot_index = (uint8_t)(slot - 1U);

            if (card_slots[slot_index].detected == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].identified == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].service.online == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if ((card_slots[slot_index].board_info.capabilities.flags & CAP_DIGITAL_OUTPUT) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (output >= card_slots[slot_index].board_info.capabilities.digital_output_count)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            frame[6] = card_manager_get_digital_output(slot, output);

            length = 1U;
            status = PROTOCOL_STATUS_OK;

            break;
        }
        case PROTOCOL_CMD_SET_DIGITAL_OUTPUT:
        {
            if (request->length != 3U)
            {
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t output = request->data[1];
            uint8_t state = request->data[2];

            if (card_manager_set_digital_output(slot, request->sequence, output, state) == 0U)
            {
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_GET_PWM:
        {
            if (request->length != 2U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t pwm = request->data[1];

            if ((slot == 0U) || (slot > slot_count))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot_index = (uint8_t)(slot - 1U);

            if (card_slots[slot_index].detected == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].identified == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_slots[slot_index].service.online == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if ((card_slots[slot_index].board_info.capabilities.flags & CAP_PWM) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (pwm >= card_slots[slot_index].board_info.capabilities.pwm_count)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            frame[6] = card_manager_get_pwm(slot, pwm);

            length = 1U;
            status = PROTOCOL_STATUS_OK;

            break;
        }
        case PROTOCOL_CMD_SET_PWM:
        {
            if (request->length != 3U)
            {
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t pwm = request->data[1];
            uint8_t duty_cycle = request->data[2];

            if (card_manager_set_pwm(slot, request->sequence, pwm, duty_cycle) == 0U)
            {
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_GET_POWER_CARD_INFO:
        {
            if (request->length != 0U)
            {
                break;
            }

            uint16_t slot_offset = 6U;

            frame[slot_offset++] = (uint8_t)(power_card.detected);
            frame[slot_offset++] = (uint8_t)(power_card.fault);
            frame[slot_offset++] = (uint8_t)(power_card.identified);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.card_type >> 8);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.card_type);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.hw_major);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.hw_minor);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.year >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.year);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.month);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.day);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.serial_number >> 24U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.serial_number >> 16U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.serial_number >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.serial_number);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.major);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.minor);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.patch);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.year >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.year);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.month);
            frame[slot_offset++] = (uint8_t)(power_card.fw_version.day);
            frame[slot_offset++] = (uint8_t)(power_card.service.protocol_major);
            frame[slot_offset++] = (uint8_t)(power_card.service.protocol_minor);
            frame[slot_offset++] = (uint8_t)(power_card.service.uptime >> 24U);
            frame[slot_offset++] = (uint8_t)(power_card.service.uptime >> 16U);
            frame[slot_offset++] = (uint8_t)(power_card.service.uptime >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card.service.uptime);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.flags >> 24U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.flags >> 16U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.flags >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.flags);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.relay_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.voltage_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.current_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.frequency_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.digital_input_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.digital_output_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.pwm_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.analog_output_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.can_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.lin_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.uart_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.rs485_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.rs232_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.i2c_count);
            frame[slot_offset++] = (uint8_t)(power_card.board_info.capabilities.spi_count);
           
            length = (uint16_t)(slot_offset - 6U);
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case PROTOCOL_CMD_ADC_DATA:
        {
            if ((request->length != 0U) || (power_card_data.adc_valid == 0U))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if ((uint32_t)(millis() - power_card_data.adc_updated_ms) >= POWER_DATA_TIMEOUT_MS)
            {
                status = PROTOCOL_STATUS_TIMEOUT;
                break;
            }

            uint16_t slot_offset = 6U;

            frame[slot_offset++] = (uint8_t)(power_card_data.vin >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card_data.vin);
            frame[slot_offset++] = (uint8_t)(power_card_data.v12 >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card_data.v12);
            frame[slot_offset++] = (uint8_t)(power_card_data.v5 >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card_data.v5);
            frame[slot_offset++] = (uint8_t)(power_card_data.v33 >> 8U);
            frame[slot_offset++] = (uint8_t)(power_card_data.v33);

            length = (uint16_t)(slot_offset - 6U);
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case PROTOCOL_CMD_INA_DATA:
        {
            if ((request->length != 0U) || (power_card_data.ina_valid == 0U))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if ((uint32_t)(millis() - power_card_data.ina_updated_ms) >= POWER_DATA_TIMEOUT_MS)
            {
                status = PROTOCOL_STATUS_TIMEOUT;
                break;
            }

            uint16_t slot_offset = 6U;

            uint32_t current = (uint32_t)power_card_data.current_ma;
            uint32_t shunt = (uint32_t)power_card_data.shunt_uv_x10;

            frame[slot_offset++] = (uint8_t)(current >> 24);
            frame[slot_offset++] = (uint8_t)(current >> 16);
            frame[slot_offset++] = (uint8_t)(current >> 8);
            frame[slot_offset++] = (uint8_t)(current);
            frame[slot_offset++] = (uint8_t)(power_card_data.voltage_mv >> 24);
            frame[slot_offset++] = (uint8_t)(power_card_data.voltage_mv >> 16);
            frame[slot_offset++] = (uint8_t)(power_card_data.voltage_mv >> 8);
            frame[slot_offset++] = (uint8_t)(power_card_data.voltage_mv);
            frame[slot_offset++] = (uint8_t)(power_card_data.power_mw >> 24);
            frame[slot_offset++] = (uint8_t)(power_card_data.power_mw >> 16);
            frame[slot_offset++] = (uint8_t)(power_card_data.power_mw >> 8);
            frame[slot_offset++] = (uint8_t)(power_card_data.power_mw);
            frame[slot_offset++] = (uint8_t)(shunt >> 24);
            frame[slot_offset++] = (uint8_t)(shunt >> 16);
            frame[slot_offset++] = (uint8_t)(shunt >> 8);
            frame[slot_offset++] = (uint8_t)(shunt);

            length = (uint16_t)(slot_offset - 6U);
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case PROTOCOL_CMD_I2C_CONFIG:
        {
            if (request->length != 6U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint32_t clock_frequency = ((uint32_t)request->data[1] << 24U) | ((uint32_t)request->data[2] << 16U) | ((uint32_t)request->data[3] << 8U) | ((uint32_t)request->data[4]);
            uint8_t addressing_mode = request->data[5];

            if (card_manager_i2c_config(slot, request->sequence, clock_frequency, addressing_mode) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_I2C_WRITE:
        {
            if (request->length < 5U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t addr = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);
            uint16_t len = ((uint16_t)request->data[3] << 8U) | ((uint16_t)request->data[4]);

            if ((len == 0U) || ((uint16_t)request->length != (5U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_i2c_write(slot, request->sequence, addr, len, &request->data[5]) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_I2C_READ:
        {
            if (request->length != 5U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];

            uint16_t addr = ((uint16_t)request->data[1] << 8U) | (uint16_t)request->data[2];
            uint16_t len = ((uint16_t)request->data[3] << 8U) | (uint16_t)request->data[4];

            if ((len == 0U) || (len > RESPONSE_PROTOCOL_MAX_PAYLOAD))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_i2c_read(slot, request->sequence, addr, len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_I2C_REG_WRITE:
        {
            if (request->length < 6U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t addr = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);
            uint8_t reg = request->data[3];
            uint16_t len = ((uint16_t)request->data[4] << 8U) | ((uint16_t)request->data[5]);

            if ((len == 0U) || ((uint16_t)request->length != (6U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_i2c_reg_write(slot, request->sequence, addr, reg, len, &request->data[6]) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_I2C_REG_READ:
        {
            if (request->length != 6U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t addr = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);
            uint8_t reg = request->data[3];
            uint16_t len = ((uint16_t)request->data[4] << 8U) | ((uint16_t)request->data[5]);

            if ((len == 0U) || (len > RESPONSE_PROTOCOL_MAX_PAYLOAD))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_i2c_reg_read(slot, request->sequence, addr, reg, len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_SPI_CONFIG:
        {
            if (request->length != 9U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint32_t frequency = ((uint32_t)request->data[1] << 24U) | ((uint32_t)request->data[2] << 16U) | ((uint32_t)request->data[3] << 8U) | ((uint32_t)request->data[4]);
            uint8_t cpol = request->data[5];
            uint8_t cpha = request->data[6];
            uint8_t bit_order = request->data[7];
            uint8_t data_size = request->data[8];

            if (card_manager_spi_config(slot, request->sequence, frequency, cpol, cpha, bit_order, data_size) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_SPI_TRANSFER_BYTE:
        {
            if (request->length != 2U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint8_t tx = request->data[1];

            if (card_manager_spi_transfer_byte(slot, request->sequence, tx) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_SPI_TRANSFER_WORD:
        {
            if (request->length < 3U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t len = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);

            if ((len == 0U) || ((uint16_t)request->length != (3U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_spi_transfer_word(slot, request->sequence, &request->data[3], len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_UART_CONFIG:
        {
            if (request->length != 8U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint32_t baudrate = ((uint32_t)request->data[1] << 24U) | ((uint32_t)request->data[2] << 16U) | ((uint32_t)request->data[3] << 8U) | ((uint32_t)request->data[4]);
            uint8_t data_bits = request->data[5];
            uint8_t parity = request->data[6];
            uint8_t stop_bits = request->data[7];

            if (card_manager_uart_config(slot, request->sequence, baudrate, data_bits, parity, stop_bits) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_UART_WRITE:
        {
            if (request->length < 3U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t len = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);

            if ((len == 0U) || ((uint16_t)request->length != (3U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_uart_write(slot, request->sequence, &request->data[3], len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_UART_READ:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            if (card_manager_uart_read(slot, request->sequence) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_RS232_CONFIG:
        {
            if (request->length != 8U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint32_t baudrate = ((uint32_t)request->data[1] << 24U) | ((uint32_t)request->data[2] << 16U) | ((uint32_t)request->data[3] << 8U) | ((uint32_t)request->data[4]);
            uint8_t data_bits = request->data[5];
            uint8_t parity = request->data[6];
            uint8_t stop_bits = request->data[7];

            if (card_manager_rs232_config(slot, request->sequence, baudrate, data_bits, parity, stop_bits) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_RS232_WRITE:
        {
            if (request->length < 3U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t len = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);

            if ((len == 0U) || ((uint16_t)request->length != (3U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_rs232_write(slot, request->sequence, &request->data[3], len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_RS232_READ:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            if (card_manager_rs232_read(slot, request->sequence) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_RS485_CONFIG:
        {
            if (request->length != 8U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint32_t baudrate = ((uint32_t)request->data[1] << 24U) | ((uint32_t)request->data[2] << 16U) | ((uint32_t)request->data[3] << 8U) | ((uint32_t)request->data[4]);
            uint8_t data_bits = request->data[5];
            uint8_t parity = request->data[6];
            uint8_t stop_bits = request->data[7];

            if (card_manager_rs485_config(slot, request->sequence, baudrate, data_bits, parity, stop_bits) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_RS485_WRITE:
        {
            if (request->length < 3U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint16_t len = ((uint16_t)request->data[1] << 8U) | ((uint16_t)request->data[2]);

            if ((len == 0U) || ((uint16_t)request->length != (3U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_rs485_write(slot, request->sequence, &request->data[3], len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_RS485_READ:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            if (card_manager_rs485_read(slot, request->sequence) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }

        case PROTOCOL_CMD_CAN_CONFIG:
        {
            if (request->length != 13)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            uint32_t nominal_bitrate = ((uint32_t)request->data[1] << 24U) | ((uint32_t)request->data[2] << 16U) | ((uint32_t)request->data[3] << 8U) | ((uint32_t)request->data[4]);
            uint32_t data_bitrate = ((uint32_t)request->data[5] << 24U) | ((uint32_t)request->data[6] << 16U) | ((uint32_t)request->data[7] << 8U) | ((uint32_t)request->data[8]);
            uint8_t fd_enabled = request->data[9];
            uint8_t brs_enabled = request->data[10];
            uint8_t mode = request->data[11];
            uint8_t can_termination = request->data[12];

            if (card_manager_can_config(slot, request->sequence, nominal_bitrate, data_bitrate, fd_enabled, brs_enabled, mode, can_termination) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_CAN_STD_SEND:
        {
            if (request->length < 7U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];

            uint16_t len = ((uint16_t)request->data[1] << 8U) | (uint16_t)request->data[2];

            if ((len < 4U) || (len > 12U) || (request->length != (uint16_t)(3U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_can_std_write(slot, request->sequence, &request->data[3], len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_CAN_FD_SEND:
        {
            if (request->length < 7U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];

            uint16_t len = ((uint16_t)request->data[1] << 8U) | (uint16_t)request->data[2];

            if ((len < 4U) || (len > 52U) || (request->length != (uint16_t)(3U + len)))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (card_manager_can_fd_write(slot, request->sequence, &request->data[3], len) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_CAN_RECEIVE:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            if (card_manager_can_read(slot, request->sequence) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }
        case PROTOCOL_CMD_MULTIPART_TEST:
        {
            if (request->length != 1U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t slot = request->data[0];
            if (card_manager_multipart_test(slot, request->sequence) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            return PROTOCOL_RESPONSE_PENDING;
        }

        default:
        {
            status = PROTOCOL_STATUS_ERROR;
            break;
        }
    }

    frame[1] = (uint8_t)(length >> 8U);
    frame[2] = (uint8_t)(length & 0xFFU);
    frame[5] = (uint8_t)status;

    uint16_t crc = crc16_ccitt(&frame[1], (uint16_t)(5U + length));

    frame[6U + length] = (uint8_t)(crc >> 8U);
    frame[7U + length] = (uint8_t)(crc & 0xFFU);

    return (uint16_t)(8U + length);
}


void tcp_protocol_rx(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return;
    }

    for (uint16_t i = 0U; i < length; i++)
    {
        uint8_t byte = data[i];

        if (rx_index == 0U)
        {
            if (byte != PROTOCOL_MAGIC)
            {
                continue;
            }

            rx_frame[rx_index++] = byte;
            continue;
        }

        if (rx_index >= PROTOCOL_REQUEST_MAX)
        {
            protocol_rx_reset();

            if (byte == PROTOCOL_MAGIC)
            {
                rx_frame[rx_index++] = byte;
            }

            continue;
        }

        rx_frame[rx_index++] = byte;

        if (rx_index == 3U)
        {
            rx_payload_length = ((uint16_t)rx_frame[1] << 8U) | (uint16_t)rx_frame[2];

            if (rx_payload_length > PROTOCOL_MAX_PAYLOAD)
            {
                protocol_rx_reset();
                continue;
            }

            rx_expected_length =
                (uint16_t)(7U + rx_payload_length);
        }

        if ((rx_expected_length == 0U) ||
            (rx_index < rx_expected_length))
        {
            continue;
        }

        uint16_t received_crc = ((uint16_t)rx_frame[rx_expected_length - 2U] << 8U) | (uint16_t)rx_frame[rx_expected_length - 1U];
        uint16_t calculated_crc = crc16_ccitt(&rx_frame[1], (uint16_t)(4U + rx_payload_length));

        if (received_crc == calculated_crc)
        {
            protocol_request_t request;

            request.length = rx_payload_length;
            request.sequence = rx_frame[3];
            request.command = rx_frame[4];

            for (uint16_t j = 0U; j < rx_payload_length; j++)
            {
                request.data[j] = rx_frame[5U + j];
            }

            uint16_t response_length = protocol_process_request(&request, response_frame);

            if (response_length != 0U)
            {
                controller_tcp_send(response_frame,  response_length);
            }
        }

        protocol_rx_reset();
    }
}