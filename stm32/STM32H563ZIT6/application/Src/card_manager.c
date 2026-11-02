#include "card_manager.h"
#include "input.h"
#include "output.h"
#include "can.h"
#include "timer.h"


volatile uint8_t slot0_detect_changed = 0U;
volatile uint8_t slot0_fault_changed = 0U;

volatile uint8_t slot1_detect_changed = 0U;
volatile uint8_t slot1_fault_changed = 0U;

volatile uint8_t slot2_detect_changed = 0U;
volatile uint8_t slot2_fault_changed = 0U;

volatile card_slot_t card_slots[3] = {0};

#define CARD_RESET_MAX_ATTEMPTS        2U
#define CARD_RESET_STABLE_HEARTBEATS  10U

typedef enum
{
    CARD_RESET_IDLE = 0U,
    CARD_RESET_ASSERTED,
    CARD_RESET_BOOT_WAIT

} card_reset_state_t;

typedef struct
{
    card_reset_state_t state;
    uint8_t attempts;
    uint8_t stable_heartbeats;

} card_reset_control_t;

static card_reset_control_t card_reset_ctrl[SLOT_COUNT] = {0};

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

static void card_reset_pin_set(uint8_t slot, uint8_t asserted)
{
    switch (slot)
    {
        case 0U:
            if (asserted != 0U)
                output_off(GPIOC, SLOT0_RESET_PIN);
            else
                output_on(GPIOC, SLOT0_RESET_PIN);
            break;

        case 1U:
            if (asserted != 0U)
                output_off(GPIOE, SLOT1_RESET_PIN);
            else
                output_on(GPIOE, SLOT1_RESET_PIN);
            break;

        case 2U:
            if (asserted != 0U)
                output_off(GPIOE, SLOT2_RESET_PIN);
            else
                output_on(GPIOE, SLOT2_RESET_PIN);
            break;

        default:
            break;
    }
}

static void card_identification_request(uint8_t slot)
{
    switch (slot)
    {
        case 0U:
            slot0_detect_changed = 1U;
            break;

        case 1U:
            slot1_detect_changed = 1U;
            break;

        case 2U:
            slot2_detect_changed = 1U;
            break;

        default:
            break;
    }
}

static void card_reset_start(uint8_t slot)
{
    if (slot >= SLOT_COUNT)
    {
        return;
    }

    if (card_slots[slot].detected == 0U)
    {
        return;
    }

    if (card_reset_ctrl[slot].state != CARD_RESET_IDLE)
    {
        return;
    }

    if (card_reset_ctrl[slot].attempts >= CARD_RESET_MAX_ATTEMPTS)
    {
        return;
    }

    card_reset_ctrl[slot].attempts++;
    card_reset_ctrl[slot].stable_heartbeats = 0U;

    card_slots[slot].identified = 0U;
    card_slots[slot].board_info = (board_info_t){0};
    card_slots[slot].fw_version = (fw_version_t){0};
    card_slots[slot].switch_status = (switch_card_status_t){0};
    card_slots[slot].service = (card_service_status_t){0};

    card_reset_pin_set(slot, 1U);

    card_reset_ctrl[slot].state = CARD_RESET_ASSERTED;
}

static void card_reset_cancel(uint8_t slot)
{
    if (slot >= SLOT_COUNT)
    {
        return;
    }

    card_reset_pin_set(slot, 0U);

    card_reset_ctrl[slot] = (card_reset_control_t){0};
}

void card_manager_init(void)
{
    gpio_output_init(GPIOC, SLOT0_RESET_PIN);
    gpio_output_init(GPIOE, SLOT1_RESET_PIN);
    gpio_output_init(GPIOE, SLOT2_RESET_PIN);

    output_on(GPIOC, SLOT0_RESET_PIN);
    output_on(GPIOE, SLOT1_RESET_PIN);
    output_on(GPIOE, SLOT2_RESET_PIN);

    gpio_output_init(GPIOC, SYNC_PIN);
    output_off(GPIOC, SYNC_PIN);

    gpio_input_init(GPIOC, SLOT0_FAULT_PIN);
    gpio_input_init(GPIOC, SLOT0_DETECT_PIN);
    
    gpio_input_init(GPIOE, SLOT1_FAULT_PIN);
    gpio_input_init(GPIOE, SLOT1_DETECT_PIN);
    
    gpio_input_init(GPIOE, SLOT2_FAULT_PIN);
    gpio_input_init(GPIOE, SLOT2_DETECT_PIN);

    EXTI->IMR1 &= ~(0x1U << SLOT0_FAULT_PIN);
    EXTI->IMR1 &= ~(0x1U << SLOT0_DETECT_PIN);

    EXTI->IMR1 &= ~(0x1U << SLOT1_FAULT_PIN);
    EXTI->IMR1 &= ~(0x1U << SLOT1_DETECT_PIN);
    
    EXTI->IMR1 &= ~(0x1U << SLOT2_FAULT_PIN);
    EXTI->IMR1 &= ~(0x1U << SLOT2_DETECT_PIN);

    EXTI->EXTICR[2] &= ~(0xFFU << 24U);
    EXTI->EXTICR[2] |=  (0x2U  << 24U);

    EXTI->EXTICR[2] &= ~(0xFFU << 16U);
    EXTI->EXTICR[2] |=  (0x2U  << 16U);

    EXTI->EXTICR[1] &= ~(0xFFU << 0U);
    EXTI->EXTICR[1] |=  (0x4U  << 0U);

    EXTI->EXTICR[1] &= ~(0xFFU << 8U);
    EXTI->EXTICR[1] |=  (0x4U  << 8U);

    EXTI->EXTICR[1] &= ~(0xFFU << 24U);
    EXTI->EXTICR[1] |=  (0x4U  << 24U);

    EXTI->EXTICR[2] &= ~(0xFFU << 0U);
    EXTI->EXTICR[2] |=  (0x4U  << 0U);

    EXTI->RTSR1 &= ~(0x1U << SLOT0_FAULT_PIN);
    EXTI->FTSR1 &= ~(0x1U << SLOT0_FAULT_PIN);
    EXTI->RTSR1 &= ~(0x1U << SLOT0_DETECT_PIN);
    EXTI->FTSR1 &= ~(0x1U << SLOT0_DETECT_PIN);
    EXTI->RTSR1 &= ~(0x1U << SLOT1_FAULT_PIN);
    EXTI->FTSR1 &= ~(0x1U << SLOT1_FAULT_PIN);
    EXTI->RTSR1 &= ~(0x1U << SLOT1_DETECT_PIN);
    EXTI->FTSR1 &= ~(0x1U << SLOT1_DETECT_PIN);
    EXTI->RTSR1 &= ~(0x1U << SLOT2_FAULT_PIN);
    EXTI->FTSR1 &= ~(0x1U << SLOT2_FAULT_PIN);
    EXTI->RTSR1 &= ~(0x1U << SLOT2_DETECT_PIN);
    EXTI->FTSR1 &= ~(0x1U << SLOT2_DETECT_PIN);

    EXTI->RTSR1 |= (0x1U << SLOT0_FAULT_PIN);
    EXTI->FTSR1 |= (0x1U << SLOT0_FAULT_PIN);
    EXTI->RPR1 = (0x1U << SLOT0_FAULT_PIN);
    EXTI->FPR1 = (0x1U << SLOT0_FAULT_PIN);
    EXTI->RTSR1 |= (0x1U << SLOT0_DETECT_PIN);
    EXTI->FTSR1 |= (0x1U << SLOT0_DETECT_PIN);
    EXTI->RPR1 = (0x1U << SLOT0_DETECT_PIN);
    EXTI->FPR1 = (0x1U << SLOT0_DETECT_PIN);
    EXTI->RTSR1 |= (0x1U << SLOT1_FAULT_PIN);
    EXTI->FTSR1 |= (0x1U << SLOT1_FAULT_PIN);
    EXTI->RPR1 = (0x1U << SLOT1_FAULT_PIN);
    EXTI->FPR1 = (0x1U << SLOT1_FAULT_PIN);
    EXTI->RTSR1 |= (0x1U << SLOT1_DETECT_PIN);
    EXTI->FTSR1 |= (0x1U << SLOT1_DETECT_PIN);
    EXTI->RPR1 = (0x1U << SLOT1_DETECT_PIN);
    EXTI->FPR1 = (0x1U << SLOT1_DETECT_PIN);
    EXTI->RTSR1 |= (0x1U << SLOT2_FAULT_PIN);
    EXTI->FTSR1 |= (0x1U << SLOT2_FAULT_PIN);
    EXTI->RPR1 = (0x1U << SLOT2_FAULT_PIN);
    EXTI->FPR1 = (0x1U << SLOT2_FAULT_PIN);
    EXTI->RTSR1 |= (0x1U << SLOT2_DETECT_PIN);
    EXTI->FTSR1 |= (0x1U << SLOT2_DETECT_PIN);
    EXTI->RPR1 = (0x1U << SLOT2_DETECT_PIN);
    EXTI->FPR1 = (0x1U << SLOT2_DETECT_PIN);

    NVIC_SetPriority(EXTI10_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI10_IRQn);
    NVIC_EnableIRQ(EXTI10_IRQn);

    NVIC_SetPriority(EXTI11_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI11_IRQn);
    NVIC_EnableIRQ(EXTI11_IRQn);

    NVIC_SetPriority(EXTI4_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI4_IRQn);
    NVIC_EnableIRQ(EXTI4_IRQn);

    NVIC_SetPriority(EXTI5_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI5_IRQn);
    NVIC_EnableIRQ(EXTI5_IRQn);

    NVIC_SetPriority(EXTI7_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI7_IRQn);
    NVIC_EnableIRQ(EXTI7_IRQn);

    NVIC_SetPriority(EXTI8_IRQn, 5U);
    NVIC_ClearPendingIRQ(EXTI8_IRQn);
    NVIC_EnableIRQ(EXTI8_IRQn);

    EXTI->IMR1 |= (0x1U << SLOT0_FAULT_PIN);
    EXTI->IMR1 |= (0x1U << SLOT0_DETECT_PIN);
    EXTI->IMR1 |= (0x1U << SLOT1_FAULT_PIN);
    EXTI->IMR1 |= (0x1U << SLOT1_DETECT_PIN);
    EXTI->IMR1 |= (0x1U << SLOT2_FAULT_PIN);
    EXTI->IMR1 |= (0x1U << SLOT2_DETECT_PIN);

    card_slots[0].fault = ((GPIOC->IDR & (1U << SLOT0_FAULT_PIN)) == 0U) ? 0U : 1U;
    card_slots[0].detected = ((GPIOC->IDR & (1U << SLOT0_DETECT_PIN)) == 0U) ? 1U : 0U;

    if (card_slots[0].detected != 0U)
    {
        slot0_detect_changed = 1U;
    }

    card_slots[1].fault = ((GPIOE->IDR & (1U << SLOT1_FAULT_PIN)) == 0U) ? 0U : 1U;
    card_slots[1].detected = ((GPIOE->IDR & (1U << SLOT1_DETECT_PIN)) == 0U) ? 1U : 0U;

    if (card_slots[1].detected != 0U)
    {
        slot1_detect_changed = 1U;
    }

    card_slots[2].fault = ((GPIOE->IDR & (1U << SLOT2_FAULT_PIN)) == 0U) ? 0U : 1U;
    card_slots[2].detected = ((GPIOE->IDR & (1U << SLOT2_DETECT_PIN)) == 0U) ? 1U : 0U;

    if (card_slots[2].detected != 0U)
    {
        slot2_detect_changed = 1U;
    }
}

void card_manager_1s_process(void)
{
    for (uint8_t i = 0U; i < SLOT_COUNT; i++)
    {
        if (card_slots[i].detected == 0U)
        {
            continue;
        }

        if (card_reset_ctrl[i].state == CARD_RESET_ASSERTED)
        {
            card_reset_pin_set(i, 0U);
            card_reset_ctrl[i].state = CARD_RESET_BOOT_WAIT;
            continue;
        }

        if (card_reset_ctrl[i].state == CARD_RESET_BOOT_WAIT)
        {
            card_reset_ctrl[i].state = CARD_RESET_IDLE;
            card_identification_request(i);
            continue;
        }

        if (card_slots[i].service.heartbeat_seen != 0U)
        {
            card_slots[i].service.heartbeat_seen = 0U;
            card_slots[i].service.missed_heartbeats = 0U;
            card_slots[i].service.online = 1U;

            if (card_reset_ctrl[i].stable_heartbeats < CARD_RESET_STABLE_HEARTBEATS)
            {
                card_reset_ctrl[i].stable_heartbeats++;
            }

            if (card_reset_ctrl[i].stable_heartbeats >= CARD_RESET_STABLE_HEARTBEATS)
            {
                card_reset_ctrl[i].attempts = 0U;
            }
        }
        else
        {
            card_reset_ctrl[i].stable_heartbeats = 0U;

            if (card_slots[i].service.missed_heartbeats < 0xFFU)
            {
                card_slots[i].service.missed_heartbeats++;
            }

            if (card_slots[i].service.missed_heartbeats >= CARD_HEARTBEAT_MISSED_MAX)
            {
                card_slots[i].service.online = 0U;
                card_reset_start(i);
            }
        }
    }
}

void card_manager_process(void)
{

    can_protocol_request_t request = {0};
    request.command = 0x02;
    request.sequence = 0x01;
    request.length = 0x00;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);

    if (slot0_fault_changed != 0U)
    {
        slot0_fault_changed = 0U;

        card_slots[0].fault = ((GPIOC->IDR & (1U << SLOT0_FAULT_PIN)) != 0U) ? 1U : 0U;
    }

    if (slot0_detect_changed != 0U)
    {
        card_slots[0].detected = ((GPIOC->IDR & (1U << SLOT0_DETECT_PIN)) == 0U) ? 1U : 0U;

        if (card_slots[0].detected != 0U)
        {
            card_slots[0].identified = 0U;

            if (can_send_std(0x100U, (const uint8_t *)&request, fd_length) != 0U)
            {
                slot0_detect_changed = 0U;
            }
        }
        else
        {
            slot0_detect_changed = 0U;
            card_reset_cancel(0U);
            card_slots[0] = (card_slot_t){0};
        }
    }

    if (slot1_fault_changed != 0U)
    {
        slot1_fault_changed = 0U;

        card_slots[1].fault = ((GPIOE->IDR & (1U << SLOT1_FAULT_PIN)) != 0U) ? 1U : 0U;
    }

    if (slot1_detect_changed != 0U)
    {
        card_slots[1].detected = ((GPIOE->IDR & (1U << SLOT1_DETECT_PIN)) == 0U) ? 1U : 0U;

        if (card_slots[1].detected != 0U)
        {
            card_slots[1].identified = 0U;

            if (can_send_std(0x200U, (const uint8_t *)&request, fd_length) != 0U)
            {
                slot1_detect_changed = 0U;
            }
        }
        else
        {
            slot1_detect_changed = 0U;
            card_reset_cancel(1U);
            card_slots[1] = (card_slot_t){0};
        }
    }

    if (slot2_fault_changed != 0U)
    {
        slot2_fault_changed = 0U;

        card_slots[2].fault = ((GPIOE->IDR & (1U << SLOT2_FAULT_PIN)) != 0U) ? 1U : 0U;
    }

    if (slot2_detect_changed != 0U)
    {
        card_slots[2].detected = ((GPIOE->IDR & (1U << SLOT2_DETECT_PIN)) == 0U) ? 1U : 0U;

        if (card_slots[2].detected != 0U)
        {
            card_slots[2].identified = 0U;

            if (can_send_std(0x300U, (const uint8_t *)&request, fd_length) != 0U)
            {
                slot2_detect_changed = 0U;
            }
        }
        else
        {
            slot2_detect_changed = 0U;
            card_reset_cancel(2U);
            card_slots[2] = (card_slot_t){0};
        }
    }
}

static void can_protocol_process_response(uint8_t slot_number, const can_protocol_response_t *response, uint8_t can_length)
{
    if (can_length < 4U)
    {
        return;
    }

    if (response->status != CAN_PROTOCOL_STATUS_OK)
    {
        return;
    }

    if (response->length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
    {
        return;
    }

    if ((4U + response->length) > can_length)
    {
        return;
    }

    switch (response->command)
    {
        case 0x02: // Read Device Info
        {
            if (response->length < 19U)
            {
                return;
            }

            board_info_t board_info = {0};
            fw_version_t fw_version = {0};

            board_info.hw_major = (uint8_t)response->data[0];
            board_info.hw_minor = (uint8_t)response->data[1];
            board_info.year = ((uint16_t)response->data[2] << 8U) | (uint16_t)response->data[3];
            board_info.month = (uint8_t)response->data[4];
            board_info.day = (uint8_t)response->data[5];
            board_info.serial_number = ((uint32_t)response->data[6] << 24U) | ((uint32_t)response->data[7] << 16U) | ((uint32_t)response->data[8] << 8U) | (uint32_t)response->data[9];
            board_info.card_type = ((uint16_t)response->data[10] << 8U) | (uint16_t)response->data[11];

            fw_version.major = (uint8_t)response->data[12];
            fw_version.minor = (uint8_t)response->data[13];
            fw_version.patch = (uint8_t)response->data[14];
            fw_version.year = ((uint16_t)response->data[15] << 8U) | (uint16_t)response->data[16];
            fw_version.month = (uint8_t)response->data[17];
            fw_version.day = (uint8_t)response->data[18];

            if ((card_slots[slot_number].detected != 0U))
            {
                card_slots[slot_number].board_info = board_info;
                card_slots[slot_number].fw_version = fw_version;
                card_slots[slot_number].identified = 1U;
            }
            break;
        }
        default:
            break;
    }
}

static void process_switch_status(uint8_t slot_number, const uint8_t *data, uint8_t length)
{
    if (length < 2U)
    {
        return;
    }

    if (card_slots[slot_number].detected == 0U)
    {
        return;
    }

    if (card_slots[slot_number].identified == 0U)
    {
        return;
    }

    if (card_slots[slot_number].board_info.card_type != CARD_TYPE_SWITCH)
    {
        return;
    }

    card_slots[slot_number].switch_status.relay_mask = ((uint16_t)data[0] << 8U) | (uint16_t)data[1];
}

static void process_service_status(uint8_t slot_number, const uint8_t *data, uint8_t length)
{
    if (length < 12U)
    {
        return;
    }

    if (card_slots[slot_number].detected == 0U)
    {
        return;
    }

    volatile card_service_status_t *service = &card_slots[slot_number].service;

    service->heartbeat_counter = data[0];
    service->status_flags      = data[1];

    service->fault_flags = ((uint16_t)data[2] << 8U) | (uint16_t)data[3];

    service->reset_reason = data[4];
    service->can_state    = data[5];

    service->uptime = ((uint32_t)data[6] << 24U) | ((uint32_t)data[7] << 16U) | ((uint32_t)data[8] << 8U)  | (uint32_t)data[9];

    service->protocol_major = data[10];
    service->protocol_minor = data[11];

    service->online = 1U;
    service->heartbeat_seen = 1U;
    service->missed_heartbeats = 0U;
}

void card_manager_can_receive(uint32_t can_id, uint8_t *can_data, uint8_t can_length)
{
    uint16_t frame_type = (uint16_t)(can_id & 0x0FFU);
    uint8_t slot_number = (uint8_t)((can_id >> 8U) - 1U);

    if (slot_number >= SLOT_COUNT)
    {
        return;
    }

    if (frame_type == 0x01U)
    {
        can_protocol_process_response(slot_number, (can_protocol_response_t *)can_data, can_length);
    }
    else if (frame_type == 0x10U)
    {
        process_service_status(slot_number, can_data, can_length);
    }
    else if (frame_type == 0x20U)
    {
        process_switch_status(slot_number, can_data, can_length);
    }
}

uint8_t card_manager_get_relay_state(uint8_t slot, uint8_t relay)
{
    if ((slot >= SLOT_COUNT) || (relay >= 12U))
    {
        return 0U;
    }

    return (uint8_t)(
        (card_slots[slot].switch_status.relay_mask >> relay) & 0x01U
    );
}

void EXTI10_IRQHandler(void)
{
    uint32_t mask = (1U << SLOT0_FAULT_PIN);

    if ((EXTI->FPR1 & mask) != 0U)
    {
        EXTI->FPR1 = mask;
        slot0_fault_changed = 1U;
    }

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;
        slot0_fault_changed = 1U;
    }
}

void EXTI11_IRQHandler(void)
{
    uint32_t mask = (1U << SLOT0_DETECT_PIN);

    if ((EXTI->FPR1 & mask) != 0U)
    {
        EXTI->FPR1 = mask;
        slot0_detect_changed = 1U;
    }

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;
        slot0_detect_changed = 1U;
    }
}

void EXTI4_IRQHandler(void)
{
    uint32_t mask = (1U << SLOT1_FAULT_PIN);

    if ((EXTI->FPR1 & mask) != 0U)
    {
        EXTI->FPR1 = mask;
        slot1_fault_changed = 1U;
    }

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;
        slot1_fault_changed = 1U;
    }
}

void EXTI5_IRQHandler(void)
{
    uint32_t mask = (1U << SLOT1_DETECT_PIN);

    if ((EXTI->FPR1 & mask) != 0U)
    {
        EXTI->FPR1 = mask;
        slot1_detect_changed = 1U;
    }

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;
        slot1_detect_changed = 1U;
    }
}

void EXTI7_IRQHandler(void)
{
    uint32_t mask = (1U << SLOT2_FAULT_PIN);

    if ((EXTI->FPR1 & mask) != 0U)
    {
        EXTI->FPR1 = mask;
        slot2_fault_changed = 1U;
    }

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;
        slot2_fault_changed = 1U;
    }
}

void EXTI8_IRQHandler(void)
{
    uint32_t mask = (1U << SLOT2_DETECT_PIN);

    if ((EXTI->FPR1 & mask) != 0U)
    {
        EXTI->FPR1 = mask;
        slot2_detect_changed = 1U;
    }

    if ((EXTI->RPR1 & mask) != 0U)
    {
        EXTI->RPR1 = mask;
        slot2_detect_changed = 1U;
    }
}