#include <stdio.h>
#include "protocol.h"
#include "support.h"
#include "input.h"
#include "can.h"
#include "led.h"
#include "relay.h"
#include "version.h"
#include "fault.h"
#include "sync.h"

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

uint8_t can_protocol_process_request(const can_protocol_request_t *request, can_protocol_response_t *frame)
{
    comm_led_activity();
    uint8_t length = 0U;
    uint8_t status;

    frame->command = request->command;
    frame->sequence = request->sequence;

    switch (request->command) {
        case 0x00: // Protocol Test
        {
            frame->data[0] = 0x11;
            frame->data[1] = 0x22;
            frame->data[2] = 0x33;
            length = 3U;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case 0x01: // Identify
        {
            identify_start();
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case 0x02: // Get Device Info
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
        case 0x10: // Get Relay
        {
            uint16_t relay_mask = relay_get_mask();

            frame->data[0] = (uint8_t)(relay_mask >> 8U);
            frame->data[1] = (uint8_t)relay_mask;

            length = 2U;
            status = PROTOCOL_STATUS_OK;
            break;
        }
        case 0x11: // Set Relay
        {
            if (request->length != 2U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            uint8_t relay_number = request->data[0];
            uint8_t relay_state  = request->data[1];

            if ((relay_number >= relay_max) || (relay_state > 1U))
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            if (relay_set_state(relay_number, relay_state) == 0U)
            {
                status = PROTOCOL_STATUS_ERROR;
                break;
            }

            frame->data[0] = relay_number;
            frame->data[1] = relay_get_state(relay_number);

            length = 2U;
            status = PROTOCOL_STATUS_OK;
            break;
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

   uint8_t sent = can_send_fd(can_id_make(slot_id, 0x10U), data_can, sizeof(data_can));

    if (sent != 0U)
    {
        heartbeat_counter++;
    }

    return sent;
}
// END Service Cyclic Frames

// Switch Board Cyclic Frames
static uint8_t send_frame_0xX20(void)
{
    uint16_t relay_mask = relay_get_mask();

    uint8_t data_can[2];

    data_can[0] = (uint8_t)(relay_mask >> 8U);
    data_can[1] = (uint8_t)relay_mask;

    return can_send_fd(can_id_make(slot_id, 0x20U), data_can, sizeof(data_can));
}
// END Switch Board Cyclic Frames

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