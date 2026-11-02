#include "protocol.h"
#include "support.h"
#include "adc.h"
#include "fault.h"
#include "ina.h"
#include "version.h"
#include <stddef.h>

static uint8_t service_reset_reason = 0U;

static void put_u16_be(uint8_t *dst, uint16_t value)
{
    dst[0] = (uint8_t)(value >> 8);
    dst[1] = (uint8_t)value;
}


static void put_u32_be(uint8_t *dst, uint32_t value)
{
    dst[0] = (uint8_t)(value >> 24);
    dst[1] = (uint8_t)(value >> 16);
    dst[2] = (uint8_t)(value >> 8);
    dst[3] = (uint8_t)value;
}

void protocol_service_init(const reset_reason_t *reason)
{
    if (reason != NULL)
    {
        service_reset_reason = reset_reason_pack(reason);
    }
    else
    {
        service_reset_reason = 0U;
    }
}

// static uint8_t service_status_flags_get(void)
// {
//     uint8_t flags = 0U;

//     if (board_info_valid() != 0U)
//     {
//         flags |= STATUS_READY;
//     }

//     if (fault_active() != 0U)
//     {
//         flags |= STATUS_FAULT;
//     }

//     if ((service_reset_reason & RESET_REASON_IWDG) != 0U)
//     {
//         flags |= STATUS_WATCHDOG_RESET;
//     }

//     return flags;
// }

uint8_t protocol_parse_request(const uint8_t *frame, uint16_t frame_length, protocol_request_t *request)
{
    if ((frame == NULL) || (request == NULL))
    {
        return PROTOCOL_STATUS_ERROR;
    }

    if (frame_length < 7U)
    {
        return PROTOCOL_STATUS_ERROR;
    }

    if (frame[0] != MAGIC)
    {
        return PROTOCOL_STATUS_ERROR;
    }

    if (frame[1] != POWER_BOARD_ADDRESS)
    {
        return PROTOCOL_STATUS_ERROR;
    }

    uint8_t length = frame[4];

    if (length > REQUEST_PROTOCOL_MAX_PAYLOAD)
    {
        return PROTOCOL_STATUS_ERROR;
    }

    if (frame_length != (uint16_t)(7U + length))
    {
        return PROTOCOL_STATUS_ERROR;
    }

    uint16_t crc = crc16_ccitt(&frame[1], (uint16_t)(4U + length));
    uint16_t received_crc = ((uint16_t)frame[5U + length] << 8) | (uint16_t)frame[6U + length];

    if (received_crc != crc)
    {
        return PROTOCOL_STATUS_ERROR;
    }

    request->magic = frame[0];
    request->address = frame[1];
    request->command = frame[2];
    request->sequence = frame[3];
    request->length = length;

    for (uint8_t i = 0U; i < length; i++)
    {
        request->data[i] = frame[5U + i];
    }

    return PROTOCOL_STATUS_OK;
}

void protocol_process_request(const protocol_request_t *request, protocol_response_t *response)
{
    if ((request == NULL) || (response == NULL))
    {
        return;
    }

    response->magic = MAGIC;
    response->address = POWER_BOARD_ADDRESS;
    response->command = request->command;
    response->sequence = request->sequence;
    response->status = PROTOCOL_STATUS_ERROR;
    response->length = 0U;

    if (request->length != 0U)
    {
        return;
    }

    switch (request->command)
    {
        case CMD_PING:
        {
            response->data[0] = 0x11U;
            response->data[1] = 0x22U;
            response->data[2] = 0x33U;

            response->length = 3U;
            response->status = PROTOCOL_STATUS_OK;

            break;
        }

        case CMD_BOARD_INFO:
        {
            const board_info_t *info = board_info_get();

            if (board_info_valid())
            {
                fault_clear(FAULT_BOARD_INFO);
                response->data[0] = info->hw_major;
                response->data[1] = info->hw_minor;
                put_u16_be(&response->data[2], info->year);
                response->data[4] = info->month;
                response->data[5] = info->day;
                put_u32_be(&response->data[6], info->serial_number);
                put_u16_be(&response->data[10], info->card_type);
                response->data[12] = fw_version.major;
                response->data[13] = fw_version.minor;
                response->data[14] = fw_version.patch;
                put_u16_be(&response->data[15], fw_version.year);
                response->data[17] = fw_version.month;
                response->data[18] = fw_version.day;
                put_u32_be(&response->data[19], info->capabilities.flags);
                response->data[23] = info->capabilities.relay_count;
                response->data[24] = info->capabilities.voltage_count;
                response->data[25] = info->capabilities.current_count;
                response->data[26] = info->capabilities.frequency_count;
                response->data[27] = info->capabilities.digital_input_count;
                response->data[28] = info->capabilities.digital_output_count;
                response->data[29] = info->capabilities.pwm_count;
                response->data[30] = info->capabilities.analog_output_count;
                response->data[31] = info->capabilities.can_count;
                response->data[32] = info->capabilities.lin_count;
                response->data[33] = info->capabilities.uart_count;
                response->data[34] = info->capabilities.rs485_count;
                response->data[35] = info->capabilities.rs232_count;
                response->data[36] = info->capabilities.i2c_count;
                response->data[37] = info->capabilities.spi_count;
                response->data[38] = info->slot_count;

                response->length = 39U;
                response->status = PROTOCOL_STATUS_OK;
            }
            else
            {
                fault_set(FAULT_BOARD_INFO);

                response->length = 0U;
                response->status = PROTOCOL_STATUS_ERROR;
            }

            break;
        }

        case CMD_ADC_DATA:
        {
            if (adc_data_valid == 0U)
            {
                break;
            }

            put_u16_be(&response->data[0], adc_vin);
            put_u16_be(&response->data[2], adc_v12);
            put_u16_be(&response->data[4], adc_v5);
            put_u16_be(&response->data[6], adc_v33);

            response->length = 8U;
            response->status = PROTOCOL_STATUS_OK;

            break;
        }


        case CMD_INA_DATA:
        {
            if (ina_data_valid == 0U)
            {
                break;
            }

            put_u32_be(&response->data[0], (uint32_t)ina_current_ma);
            put_u32_be(&response->data[4], ina_voltage_mv);
            put_u32_be(&response->data[8], ina_power_mw);
            put_u32_be(&response->data[12], (uint32_t)ina_shunt_voltage_uv_x10);

            response->length = 16U;
            response->status = PROTOCOL_STATUS_OK;

            break;
        }


        default:
        {
            response->length = 0U;
            response->status = PROTOCOL_STATUS_ERROR;

            break;
        }
    }
}


uint16_t protocol_build_response(const protocol_response_t *response, uint8_t *frame)
{
    if ((response == NULL) || (frame == NULL))
    {
        return 0U;
    }

    if (response->length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
    {
        return 0U;
    }

    frame[0] = response->magic;
    frame[1] = response->address;
    frame[2] = response->command;
    frame[3] = response->sequence;
    frame[4] = response->status;
    frame[5] = response->length;

    for (uint8_t i = 0U; i < response->length; i++)
    {
        frame[6U + i] = response->data[i];
    }

    uint16_t crc = crc16_ccitt(&frame[1], (uint16_t)(5U + response->length));

    frame[6U + response->length] = (uint8_t)(crc >> 8);
    frame[7U + response->length] = (uint8_t)crc;

    return (uint16_t)(8U + response->length);
}