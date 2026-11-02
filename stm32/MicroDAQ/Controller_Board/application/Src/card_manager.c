#include "card_manager.h"
#include "input.h"
#include "output.h"
#include "can.h"
#include "timer.h"
#include "uart.h"

uint8_t slot_count = 0U;

volatile uint8_t slot0_detect_changed = 0U;
volatile uint8_t slot0_fault_changed = 0U;

volatile uint8_t slot1_detect_changed = 0U;
volatile uint8_t slot1_fault_changed = 0U;

volatile uint8_t slot2_detect_changed = 0U;
volatile uint8_t slot2_fault_changed = 0U;

volatile card_slot_t card_slots[SLOT_COUNT_MAX] = {0};
volatile card_slot_t power_card = {0};
volatile power_card_t power_card_data = {0};

static uint8_t uart2_tx_buffer[PROTOCOL_REQUEST_FRAME_MAX];
static uint8_t uart2_sequence = 0U;

static card_transaction_t card_transaction[SLOT_COUNT_MAX + 1U];
static card_transaction_response_t transaction_response[SLOT_COUNT_MAX + 1U];

static card_transaction_multipart_response_t transaction_multipart_response[SLOT_COUNT_MAX + 1U];

static void card_transaction_clear(uint8_t slot);

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

static card_reset_control_t card_reset_ctrl[SLOT_COUNT_MAX] = {0};

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

static uint8_t card_interface_ready(uint8_t slot, uint32_t capability)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);

    if (card_slots[slot_index].detected == 0U)
    {
        return 0U;
    }

    if (card_slots[slot_index].identified == 0U)
    {
        return 0U;
    }

    if (card_slots[slot_index].service.online == 0U)
    {
        return 0U;
    }

    if ((card_slots[slot_index].board_info.capabilities.flags & capability) == 0U)
    {
        return 0U;
    }

    return 1U;
}

static void card_reset_start(uint8_t slot)
{
    if (slot >= slot_count)
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
    card_slots[slot].relay_status = (relay_status_t){0};
    card_slots[slot].digital_output_status = (digital_output_status_t){0};
    card_slots[slot].pwm_status = (pwm_status_t){0};
    card_slots[slot].service = (card_service_status_t){0};

    card_reset_pin_set(slot, 1U);

    card_reset_ctrl[slot].state = CARD_RESET_ASSERTED;
}

static void card_reset_cancel(uint8_t slot)
{
    if (slot >= slot_count)
    {
        return;
    }

    card_reset_pin_set(slot, 0U);

    card_reset_ctrl[slot] = (card_reset_control_t){0};
}

static void mask_set_state(uint16_t *mask, uint8_t number, uint8_t state)
{
    if (state != 0U)
    {
        *mask |= (uint16_t)(1U << number);
    }
    else
    {
        *mask &= (uint16_t)~(1U << number);
    }
}

static uint8_t card_transaction_start(uint8_t slot, uint8_t sequence, uint8_t command)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_transaction[slot].active != 0U)
    {
        return 0U;
    }

    if (transaction_response[slot].ready != 0U)
    {
        return 0U;
    }

    card_transaction[slot].active = 1U;
    card_transaction[slot].slot = slot;
    card_transaction[slot].sequence = sequence;
    card_transaction[slot].command = command;
    card_transaction[slot].start_tick = time_ms;

    return 1U;
}

static void card_transaction_clear(uint8_t slot)
{
    if ((slot == 0U) || (slot > SLOT_COUNT_MAX))
    {
        return;
    }

    card_transaction[slot].active = 0U;
    card_transaction[slot].slot = 0U;
    card_transaction[slot].sequence = 0U;
    card_transaction[slot].command = 0U;
    card_transaction[slot].start_tick = 0U;
}

void card_manager_init(void)
{
    const board_info_t *info = board_info_get();
    slot_count = 0U;

    if (board_info_valid() != 0U)
    {
        slot_count = info->slot_count;
        if (slot_count > SLOT_COUNT_MAX)
        {
            slot_count = SLOT_COUNT_MAX;
        }
    }

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
    for (uint8_t i = 0U; i < slot_count; i++)
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

            if (card_slots[i].identified == 0U)
            {
                card_identification_request(i);
            }

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
    for (uint8_t slot = 1U; slot <= slot_count; slot++)
    {
        card_transaction_t *transaction = &card_transaction[slot];

        if (transaction->active == 0U)
        {
            continue;
        }

        if ((uint32_t)(time_ms - transaction->start_tick) < CARD_TRANSACTION_TIMEOUT_MS)
        {
            continue;
        }

        card_transaction_response_t *response = &transaction_response[slot];

        if (response->ready != 0U)
        {
            continue;
        }

        response->ready = 1U;
        response->sequence = transaction->sequence;
        response->command = transaction->command;
        response->status = CAN_PROTOCOL_STATUS_ERROR;
        response->length = 0U;

        if (transaction->command == CAN_PROTOCOL_CMD_MULTIPART_TEST)
        {
            card_transaction_multipart_response_t *multipart = &transaction_multipart_response[slot];
            multipart->ready = 0U;
            multipart->length = 0U;
            multipart->count = 0U;
            multipart->package = 0U;
        }

        card_transaction_clear(slot);
    }

    can_protocol_request_t request = {0};
    request.command = CAN_PROTOCOL_CMD_GET_CARD_INFO;
    request.sequence = 0x01;
    request.length = 0x00;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);

    if (slot_count > 0U)
    {
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
    }

    if (slot_count > 1U)
    {
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
    }

    if (slot_count > 2U)
    {
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
}

static uint8_t card_transaction_response(uint8_t slot_number, const can_protocol_response_t *response)
{
    uint8_t slot = (uint8_t)(slot_number + 1U);

    if ((slot == 0U) || (slot > SLOT_COUNT_MAX))
    {
        return 0U;
    }

    card_transaction_t *transaction = &card_transaction[slot];

    if (transaction->active == 0U)
    {
        return 0U;
    }

    if (transaction->sequence != response->sequence)
    {
        return 0U;
    }

    if (transaction->command != response->command)
    {
        return 0U;
    }

    card_transaction_response_t *completed =  &transaction_response[slot];

    if (completed->ready != 0U)
    {
        return 0U;
    }

    completed->sequence = response->sequence;
    completed->command = response->command;
    completed->status = response->status;
    completed->length = response->length;

    for (uint8_t i = 0U; i < response->length; i++)
    {
        completed->data[i] = response->data[i];
    }

    card_transaction_clear(slot);
    completed->ready = 1U;

    return 1U;
}

const card_transaction_multipart_response_t * card_manager_multipart_response_get(uint8_t slot)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return NULL;
    }

    if (transaction_multipart_response[slot].ready == 0U)
    {
        return NULL;
    }

    return &transaction_multipart_response[slot];
}

void card_manager_multipart_response_clear(uint8_t slot)
{
    if ((slot == 0U) || (slot > SLOT_COUNT_MAX))
    {
        return;
    }

    transaction_multipart_response[slot].ready = 0U;
    transaction_multipart_response[slot].length = 0U;
    transaction_multipart_response[slot].count = 0U;
    transaction_multipart_response[slot].package = 0U;
}

static uint8_t card_transaction_multipart_response(uint8_t slot_number, const uint8_t *frame, uint8_t can_length)
{
    uint8_t slot = (uint8_t)(slot_number + 1U);

    if ((frame == NULL) || (slot == 0U) || (slot > slot_count) || (can_length < 8U))
    {
        return 0U;
    }

    uint8_t command = frame[0];
    uint8_t sequence = frame[1];
    uint8_t status = frame[2];
    uint8_t length = frame[3];

    uint16_t count = ((uint16_t)frame[4] << 8U) | (uint16_t)frame[5];
    uint16_t package = ((uint16_t)frame[6] << 8U) | (uint16_t)frame[7];

    card_transaction_t *transaction = &card_transaction[slot];
    card_transaction_multipart_response_t *completed = &transaction_multipart_response[slot];

    if ((status != CAN_PROTOCOL_STATUS_MULTIPART) || (length == 0U) || (length > MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD) || ((uint16_t)(8U + length) > can_length) || (count == 0U) || (package == 0U) || (package > count))
    {
        return 0U;
    }

    if ((transaction->active == 0U) || (transaction->command != command) || (transaction->sequence != sequence) || (completed->ready != 0U))
    {
        return 0U;
    }

    if ((package < count) && (length != MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD))
    {
        return 0U;
    }

    uint32_t offset = ((uint32_t)package - 1U) * MULTIPART_RESPONSE_PROTOCOL_MAX_PAYLOAD;

    if ((offset + length) > sizeof(completed->data))
    {
        return 0U;
    }

    if (package == 1U)
    {
        if (completed->package != 0U)
        {
            return 0U;
        }

        completed->command = command;
        completed->sequence = sequence;
        completed->count = count;
        completed->length = 0U;
    }
    else
    {
        if ((completed->count != count) || (package != (uint16_t)(completed->package + 1U)))
        {
            return 0U;
        }
    }

    for (uint8_t i = 0U; i < length; i++)
    {
        completed->data[offset + i] = frame[8U + i];
    }

    completed->package = package;
    completed->length = (uint16_t)(offset + length);

    transaction->start_tick = time_ms;

    if (package == count)
    {
        completed->status = CAN_PROTOCOL_STATUS_OK;
        completed->ready = 1U;

        card_transaction_clear(slot);
    }

    return 1U;
}

static uint8_t can_protocol_process_response(uint8_t slot_number, const can_protocol_response_t *response, uint8_t can_length)
{
    if (can_length < 4U)
    {
        return 0U;
    }

    if (response->length > RESPONSE_PROTOCOL_MAX_PAYLOAD)
    {
        return 0U;
    }

    if ((4U + response->length) > can_length)
    {
        return 0U;
    }

    if (response->command == CAN_PROTOCOL_CMD_MULTIPART_TEST)
    {
        if (response->status == CAN_PROTOCOL_STATUS_MULTIPART)
        {
            return 0U;
        }

        return card_transaction_response(slot_number, response);
    }

    uint8_t transactional = 0U;

    if (((response->command >= CAN_PROTOCOL_CMD_I2C_CONFIG) && (response->command <= CAN_PROTOCOL_CMD_CAN_RECEIVE)) || (response->command == CAN_PROTOCOL_CMD_IDENTIFY_CARD) || (response->command == CAN_PROTOCOL_CMD_SET_RELAY) || (response->command == CAN_PROTOCOL_CMD_SET_DIGITAL_OUTPUT) || (response->command == CAN_PROTOCOL_CMD_SET_PWM))
    {
        transactional = 1U;
    }

    if (transactional != 0U)
    {
        uint8_t slot = (uint8_t)(slot_number + 1U);

        if ((card_transaction[slot].active == 0U) || (card_transaction[slot].sequence != response->sequence) || (card_transaction[slot].command != response->command))
        {
            return 0U;
        }

        if (((response->command >= CAN_PROTOCOL_CMD_I2C_CONFIG) && (response->command <= CAN_PROTOCOL_CMD_CAN_RECEIVE)) || (response->status != CAN_PROTOCOL_STATUS_OK))
        {
            return card_transaction_response(slot_number, response);
        }
    }

    if (response->status != CAN_PROTOCOL_STATUS_OK)
    {
        return 0U;
    }

    switch (response->command)
    {
        case 0x01U: // Identify response
        {
            if (response->length != 0U)
            {
                return 0U;
            }

            return card_transaction_response(slot_number, response);
        }
        case 0x02U: // Read Device Info
        {
            if (response->length < 39U)
            {
                return 0U;
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
            board_info.capabilities.flags = ((uint32_t)response->data[19] << 24U) | ((uint32_t)response->data[20] << 16U) | ((uint32_t)response->data[21] << 8U)  | ((uint32_t)response->data[22]);
            board_info.capabilities.relay_count          = response->data[23];
            board_info.capabilities.voltage_count        = response->data[24];
            board_info.capabilities.current_count        = response->data[25];
            board_info.capabilities.frequency_count      = response->data[26];
            board_info.capabilities.digital_input_count  = response->data[27];
            board_info.capabilities.digital_output_count = response->data[28];
            board_info.capabilities.pwm_count            = response->data[29];
            board_info.capabilities.analog_output_count  = response->data[30];
            board_info.capabilities.can_count             = response->data[31];
            board_info.capabilities.lin_count             = response->data[32];
            board_info.capabilities.uart_count            = response->data[33];
            board_info.capabilities.rs485_count           = response->data[34];
            board_info.capabilities.rs232_count           = response->data[35];
            board_info.capabilities.i2c_count             = response->data[36];
            board_info.capabilities.spi_count             = response->data[37];
            board_info.slot_count = response->data[38];

            if ((card_slots[slot_number].detected != 0U))
            {
                card_slots[slot_number].board_info = board_info;
                card_slots[slot_number].fw_version = fw_version;
                card_slots[slot_number].identified = 1U;
            }
            return 1U;
        }
        case 0x11U: // Set Relay response
        {
            if (response->length != 2U)
            {
                return 0U;
            }

            uint8_t relay_number = response->data[0];
            uint8_t relay_state  = response->data[1];

            if ((card_slots[slot_number].board_info.capabilities.flags & CAP_RELAY) == 0U)
            {
                return 0U;
            }

            if (relay_number >= card_slots[slot_number].board_info.capabilities.relay_count)
            {
                return 0U;
            }

            if (relay_state > 1U)
            {
                return 0U;
            }

            uint16_t relay_mask = card_slots[slot_number].relay_status.relay_mask;
            mask_set_state(&relay_mask, relay_number, relay_state);
            card_slots[slot_number].relay_status.relay_mask = relay_mask;

            return card_transaction_response(slot_number, response);
        }
        case 0x13U: // Set Digital Output response
        {
            if (response->length != 2U)
            {
                return 0U;
            }

            uint8_t output_number = response->data[0];
            uint8_t output_state  = response->data[1];

            if ((card_slots[slot_number].board_info.capabilities.flags & CAP_DIGITAL_OUTPUT) == 0U)
            {
                return 0U;
            }

            if (output_number >= card_slots[slot_number].board_info.capabilities.digital_output_count)
            {
                return 0U;
            }

            if (output_state > 1U)
            {
                return 0U;
            }

            uint16_t output_mask = card_slots[slot_number].digital_output_status.output_mask;
            mask_set_state(&output_mask, output_number, output_state);
            card_slots[slot_number].digital_output_status.output_mask = output_mask;

            return card_transaction_response(slot_number, response);
        }
        case 0x15U: // Set PWM response
        {
            if (response->length != 2U)
            {
                return 0U;
            }

            uint8_t pwm_number = response->data[0];
            uint8_t duty_cycle = response->data[1];

            if ((card_slots[slot_number].board_info.capabilities.flags & CAP_PWM) == 0U)
            {
                return 0U;
            }

            if (pwm_number >= card_slots[slot_number].board_info.capabilities.pwm_count)
            {
                return 0U;
            }

            if (duty_cycle > 100U)
            {
                return 0U;
            }

            card_slots[slot_number].pwm_status.duty_cycle[pwm_number] = duty_cycle;

            uint16_t pwm_mask = card_slots[slot_number].pwm_status.pwm_mask;
            mask_set_state(&pwm_mask, pwm_number, (duty_cycle != 0U) ? 1U : 0U);
            card_slots[slot_number].pwm_status.pwm_mask = pwm_mask;

            return card_transaction_response(slot_number, response);
        }
        default:
            break;
    }
    return 0U;
}

static uint8_t process_relay_status(uint8_t slot_number, const uint8_t *data, uint8_t length)
{
    if (length < 2U)
    {
        return 0U;
    }

    if (card_slots[slot_number].detected == 0U)
    {
        return 0U;
    }

    if (card_slots[slot_number].identified == 0U)
    {
        return 0U;
    }

    if ((card_slots[slot_number].board_info.capabilities.flags & CAP_RELAY) == 0U)
    {
        return 0U;
    }

    card_slots[slot_number].relay_status.relay_mask = ((uint16_t)data[0] << 8U) | (uint16_t)data[1];
    return 1U;
}

static uint8_t process_digital_output_status(uint8_t slot_number, const uint8_t *data, uint8_t length)
{
    if (length < 2U)
    {
        return 0U;
    }

    if (card_slots[slot_number].detected == 0U)
    {
        return 0U;
    }

    if (card_slots[slot_number].identified == 0U)
    {
        return 0U;
    }

    if ((card_slots[slot_number].board_info.capabilities.flags & CAP_DIGITAL_OUTPUT) == 0U)
    {
        return 0U;
    }

    card_slots[slot_number].digital_output_status.output_mask = ((uint16_t)data[0] << 8U) | (uint16_t)data[1];
    return 1U;
}

static uint8_t process_pwm_status(uint8_t slot_number, const uint8_t *data, uint8_t length)
{
    if (length < 2U)
    {
        return 0U;
    }

    if (card_slots[slot_number].detected == 0U)
    {
        return 0U;
    }

    if (card_slots[slot_number].identified == 0U)
    {
        return 0U;
    }

    if ((card_slots[slot_number].board_info.capabilities.flags & CAP_PWM) == 0U)
    {
        return 0U;
    }

    card_slots[slot_number].pwm_status.pwm_mask = ((uint16_t)data[0] << 8U) | (uint16_t)data[1];
    return 1U;
}

static uint8_t process_service_status(uint8_t slot_number, const uint8_t *data, uint8_t length)
{
    if (length < 12U)
    {
        return 0U;
    }

    if (card_slots[slot_number].detected == 0U)
    {
        return 0U;
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

    return 1U;
}

uint8_t card_manager_can_receive(uint32_t can_id, uint8_t *can_data, uint8_t can_length)
{
    uint16_t frame_type = (uint16_t)(can_id & 0x0FFU);
    uint8_t slot_number = (uint8_t)((can_id >> 8U) - 1U);

    if (slot_number >= slot_count)
    {
        return 0U;
    }

    if (frame_type == 0x01U)
    {
        if ((can_length >= 8U) && (can_data[2] == CAN_PROTOCOL_STATUS_MULTIPART))
        {
            return card_transaction_multipart_response(slot_number, can_data, can_length);
        }
        return can_protocol_process_response(slot_number, (const can_protocol_response_t *)can_data, can_length);
    }
    else if (frame_type == 0x10U)
    {
        return process_service_status(slot_number, can_data, can_length);
    }
    else if (frame_type == 0x20U)
    {
        return process_relay_status(slot_number, can_data, can_length);
    }
    else if (frame_type == 0x21U)
    {
        return process_digital_output_status(slot_number, can_data, can_length);
    }
    else if (frame_type == 0x22U)
    {
        return process_pwm_status(slot_number, can_data, can_length);
    }
    return 0U;
}


uint8_t card_manager_identify(uint8_t slot, uint8_t sequence)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);

    if (card_slots[slot_index].detected == 0U)
    {
        return 0U;
    }

    if (card_slots[slot_index].identified == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_IDENTIFY_CARD) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_IDENTIFY_CARD;
    request.sequence = sequence;
    request.length = 0U;

    uint8_t fd_length = can_fd_pad_length(3U);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_get_relay(uint8_t slot, uint8_t relay)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);
    return (uint8_t)( (card_slots[slot_index].relay_status.relay_mask >> relay) & 0x01U);
}

uint8_t card_manager_set_relay(uint8_t slot, uint8_t sequence, uint8_t relay, uint8_t state)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);

    if (card_interface_ready(slot, CAP_RELAY) == 0U)
    {
        return 0U;
    }

    if ((relay >= card_slots[slot_index].board_info.capabilities.relay_count) || (state > 1U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_SET_RELAY) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_SET_RELAY;
    request.sequence = sequence;
    request.length = 2U;
    request.data[0] = relay;
    request.data[1] = state;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request,  fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_get_digital_output(uint8_t slot, uint8_t output)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);
    return (uint8_t)( (card_slots[slot_index].digital_output_status.output_mask >> output) & 0x01U);
}

uint8_t card_manager_set_digital_output(uint8_t slot, uint8_t sequence, uint8_t output, uint8_t state)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);

    if (card_interface_ready(slot, CAP_DIGITAL_OUTPUT) == 0U)
    {
        return 0U;
    }

    if ((output >= card_slots[slot_index].board_info.capabilities.digital_output_count) || (state > 1U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_SET_DIGITAL_OUTPUT) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_SET_DIGITAL_OUTPUT;
    request.sequence = sequence;
    request.length = 2U;
    request.data[0] = output;
    request.data[1] = state;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request,  fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_get_pwm(uint8_t slot, uint8_t pwm)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);

    if (pwm >= card_slots[slot_index].board_info.capabilities.pwm_count)
    {
        return 0U;
    }

    return card_slots[slot_index].pwm_status.duty_cycle[pwm];
}

uint8_t card_manager_set_pwm(uint8_t slot, uint8_t sequence, uint8_t pwm, uint8_t duty_cycle)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    uint8_t slot_index = (uint8_t)(slot - 1U);

    if (card_interface_ready(slot, CAP_PWM) == 0U)
    {
        return 0U;
    }

    if ((pwm >= card_slots[slot_index].board_info.capabilities.pwm_count) || (duty_cycle > 100U))
    {
        return 0U;
    }
    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_SET_PWM) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_SET_PWM;
    request.sequence = sequence;
    request.length = 2U;
    request.data[0] = pwm;
    request.data[1] = duty_cycle;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request,  fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_uart_send(uint8_t command)
{
    uint8_t payload_length = 0U;

    uart2_tx_buffer[0] = MAGIC;
    uart2_tx_buffer[1] = POWER_BOARD_ADDRESS;
    uart2_tx_buffer[2] = command;
    uart2_tx_buffer[3] = uart2_sequence++;
    uart2_tx_buffer[4] = payload_length;

    uint16_t crc = crc16_ccitt(&uart2_tx_buffer[1], (uint16_t)(4U + payload_length));
    uart2_tx_buffer[5U + payload_length] = (uint8_t)(crc >> 8U);
    uart2_tx_buffer[6U + payload_length] = (uint8_t)(crc & 0xFFU);
    uint16_t frame_length = (uint16_t)(7U + payload_length);

    return uart2_dma_write(uart2_tx_buffer, frame_length);
}

uint8_t card_manager_uart_receive(protocol_uart_response_t *response, uint16_t length)
{
    if (response == NULL)
    {
        return 0U;
    }

    if (response->command == UART_CMD_ADC_DATA)
    {
        power_card_data.adc_valid = 0U;
    }
    else if (response->command == UART_CMD_INA_DATA)
    {
        power_card_data.ina_valid = 0U;
    }

    if (response->status != UART_PROTOCOL_STATUS_OK)
    {
        return 0U;
    }

    switch (response->command)
    {
        case UART_CMD_PING:
            if (length != 3U)
            {
                return 0U;
            }

            break;
        case UART_CMD_BOARD_INFO :
            if (length != 39U)
            {
                return 0U;
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
            board_info.capabilities.flags = ((uint32_t)response->data[19] << 24U) | ((uint32_t)response->data[20] << 16U) | ((uint32_t)response->data[21] << 8U)  | ((uint32_t)response->data[22]);
            board_info.capabilities.relay_count          = response->data[23];
            board_info.capabilities.voltage_count        = response->data[24];
            board_info.capabilities.current_count        = response->data[25];
            board_info.capabilities.frequency_count      = response->data[26];
            board_info.capabilities.digital_input_count  = response->data[27];
            board_info.capabilities.digital_output_count = response->data[28];
            board_info.capabilities.pwm_count            = response->data[29];
            board_info.capabilities.analog_output_count  = response->data[30];
            board_info.capabilities.can_count             = response->data[31];
            board_info.capabilities.lin_count             = response->data[32];
            board_info.capabilities.uart_count            = response->data[33];
            board_info.capabilities.rs485_count           = response->data[34];
            board_info.capabilities.rs232_count           = response->data[35];
            board_info.capabilities.i2c_count             = response->data[36];
            board_info.capabilities.spi_count             = response->data[37];
            board_info.slot_count = response->data[38];

            power_card.detected = 1U;
            power_card.fault = 0U;
            power_card.identified = 1U;
            power_card.board_info = board_info;
            power_card.fw_version = fw_version;
            power_card.relay_status = (relay_status_t){0};
            power_card.digital_output_status = (digital_output_status_t){0};
            power_card.pwm_status = (pwm_status_t){0};

            break;
        case UART_CMD_ADC_DATA:
            if (length != 8U)
            {
                return 0U;
            }

            power_card_data.vin = ((uint16_t)response->data[0] << 8U) | (uint16_t)response->data[1];
            power_card_data.v12 = ((uint16_t)response->data[2] << 8U) | (uint16_t)response->data[3];
            power_card_data.v5 = ((uint16_t)response->data[4] << 8U) | (uint16_t)response->data[5];
            power_card_data.v33 = ((uint16_t)response->data[6] << 8U) | (uint16_t)response->data[7];
            power_card_data.adc_updated_ms = millis();
            power_card_data.adc_valid = 1U;
            break;
        case UART_CMD_INA_DATA:
            if (length != 16U)
            {
                return 0U;
            }

            uint32_t current_raw = ((uint32_t)response->data[0] << 24) | ((uint32_t)response->data[1] << 16) | ((uint32_t)response->data[2] << 8) | (uint32_t)response->data[3];
            power_card_data.current_ma = (int32_t)current_raw;

            power_card_data.voltage_mv = ((uint32_t)response->data[4] << 24) | ((uint32_t)response->data[5] << 16) | ((uint32_t)response->data[6] << 8) | (uint32_t)response->data[7];

            power_card_data.power_mw = ((uint32_t)response->data[8] << 24) | ((uint32_t)response->data[9] << 16) | ((uint32_t)response->data[10] << 8) | (uint32_t)response->data[11];

            uint32_t shunt_raw = ((uint32_t)response->data[12] << 24) | ((uint32_t)response->data[13] << 16) | ((uint32_t)response->data[14] << 8) | (uint32_t)response->data[15];
            power_card_data.shunt_uv_x10 = (int32_t)shunt_raw;

            power_card_data.ina_updated_ms = millis();
            power_card_data.ina_valid = 1U;

            break;
        default:
            return 0U;
    }
    return 1U;
}

uint8_t card_manager_i2c_config(uint8_t slot, uint8_t sequence, uint32_t clock_frequency, uint8_t addressing_mode)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_I2C) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_I2C_CONFIG) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_I2C_CONFIG;
    request.sequence = sequence;
    request.length = 5U;

    request.data[0] = (uint8_t)(clock_frequency >> 24U);
    request.data[1] = (uint8_t)(clock_frequency >> 16U);
    request.data[2] = (uint8_t)(clock_frequency >> 8U);
    request.data[3] = (uint8_t)clock_frequency;
    request.data[4] = addressing_mode;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_i2c_write(uint8_t slot, uint8_t sequence, uint16_t addr, uint16_t length, const uint8_t *data)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_I2C) == 0U)
    {
        return 0U;
    }

    if ((data == NULL) || (length == 0U) || (length > 57U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_I2C_WRITE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_I2C_WRITE;
    request.sequence = sequence;
    request.length = 4U + length;

    request.data[0] = (uint8_t)(addr >> 8U);
    request.data[1] = (uint8_t)addr;
    request.data[2] = (uint8_t)(length >> 8U);
    request.data[3] = (uint8_t)length;

    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[4U + i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_i2c_read(uint8_t slot, uint8_t sequence, uint16_t addr, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_I2C) == 0U)
    {
        return 0U;
    }

    if ((length == 0U) || (length > RESPONSE_PROTOCOL_MAX_PAYLOAD))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_I2C_READ) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_I2C_READ;
    request.sequence = sequence;
    request.length = 4U;

    request.data[0] = (uint8_t)(addr >> 8U);
    request.data[1] = (uint8_t)addr;
    request.data[2] = (uint8_t)(length >> 8U);
    request.data[3] = (uint8_t)length;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_i2c_reg_write( uint8_t slot, uint8_t sequence, uint16_t addr, uint8_t reg, uint16_t length, const uint8_t *data)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_I2C) == 0U)
    {
        return 0U;
    }

    if ((data == NULL) || (length == 0U) || (length > 56U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_I2C_REG_WRITE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_I2C_REG_WRITE;
    request.sequence = sequence;
    request.length = 5U + length;

    request.data[0] = (uint8_t)(addr >> 8U);
    request.data[1] = (uint8_t)addr;
    request.data[2] = reg;
    request.data[3] = (uint8_t)(length >> 8U);
    request.data[4] = (uint8_t)length;

    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[5U + i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_i2c_reg_read( uint8_t slot, uint8_t sequence, uint16_t addr, uint8_t reg, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_I2C) == 0U)
    {
        return 0U;
    }

    if ((length == 0U) || (length > RESPONSE_PROTOCOL_MAX_PAYLOAD))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_I2C_REG_READ) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_I2C_REG_READ;
    request.sequence = sequence;
    request.length = 5U;

    request.data[0] = (uint8_t)(addr >> 8U);
    request.data[1] = (uint8_t)addr;
    request.data[2] = reg;
    request.data[3] = (uint8_t)(length >> 8U);
    request.data[4] = (uint8_t)length;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_spi_config( uint8_t slot, uint8_t sequence, uint32_t frequency, uint8_t cpol, uint8_t cpha, uint8_t bit_order, uint8_t data_size)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_SPI) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_SPI_CONFIG) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_SPI_CONFIG;
    request.sequence = sequence;
    request.length = 8U;

    request.data[0] = (uint8_t)(frequency >> 24U);
    request.data[1] = (uint8_t)(frequency >> 16U);
    request.data[2] = (uint8_t)(frequency >> 8U);
    request.data[3] = (uint8_t)frequency;
    request.data[4] = cpol;
    request.data[5] = cpha;
    request.data[6] = bit_order;
    request.data[7] = data_size;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_spi_transfer_byte(uint8_t slot, uint8_t sequence, uint8_t tx)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_SPI) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_SPI_TRANSFER_BYTE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_SPI_TRANSFER_BYTE;
    request.sequence = sequence;
    request.length = 1U;
    request.data[0] = tx;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_spi_transfer_word(uint8_t slot, uint8_t sequence, const uint8_t *tx, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_SPI) == 0U)
    {
        return 0U;
    }

    if ((tx == NULL) || (length == 0U) || (length > 59U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_SPI_TRANSFER_WORD) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_SPI_TRANSFER_WORD;
    request.sequence = sequence;
    request.length = (uint8_t)(2U + length);

    request.data[0] = (uint8_t)(length >> 8U);
    request.data[1] = (uint8_t)length;

    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[2U + i] = tx[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_uart_config(uint8_t slot, uint8_t sequence, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_UART) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_UART_CONFIG) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_UART_CONFIG;
    request.sequence = sequence;
    request.length = 7U;
    request.data[0] = (uint8_t)(baudrate >> 24U);
    request.data[1] = (uint8_t)(baudrate >> 16U);
    request.data[2] = (uint8_t)(baudrate >> 8U);
    request.data[3] = (uint8_t)(baudrate);
    request.data[4] = data_bits;
    request.data[5] = parity;
    request.data[6] = stop_bits;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_uart_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_UART) == 0U)
    {
        return 0U;
    }

    if ((data == NULL) || (length == 0U) || (length > 61U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_UART_WRITE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_UART_WRITE;
    request.sequence = sequence;
    request.length = length;
    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_uart_read(uint8_t slot, uint8_t sequence)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_UART) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_UART_READ) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_UART_READ;
    request.sequence = sequence;
    request.length = 0U;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_rs232_config(uint8_t slot, uint8_t sequence, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_RS232) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_RS232_CONFIG) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_RS232_CONFIG;
    request.sequence = sequence;
    request.length = 7U;
    request.data[0] = (uint8_t)(baudrate >> 24U);
    request.data[1] = (uint8_t)(baudrate >> 16U);
    request.data[2] = (uint8_t)(baudrate >> 8U);
    request.data[3] = (uint8_t)(baudrate);
    request.data[4] = data_bits;
    request.data[5] = parity;
    request.data[6] = stop_bits;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_rs232_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_RS232) == 0U)
    {
        return 0U;
    }

    if ((data == NULL) || (length == 0U) || (length > 61U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_RS232_WRITE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_RS232_WRITE;
    request.sequence = sequence;
    request.length = length;
    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_rs232_read(uint8_t slot, uint8_t sequence)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_RS232) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_RS232_READ) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_RS232_READ;
    request.sequence = sequence;
    request.length = 0U;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_rs485_config(uint8_t slot, uint8_t sequence, uint32_t baudrate, uint8_t data_bits, uint8_t parity, uint8_t stop_bits)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_RS485) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_RS485_CONFIG) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_RS485_CONFIG;
    request.sequence = sequence;
    request.length = 7U;
    request.data[0] = (uint8_t)(baudrate >> 24U);
    request.data[1] = (uint8_t)(baudrate >> 16U);
    request.data[2] = (uint8_t)(baudrate >> 8U);
    request.data[3] = (uint8_t)(baudrate);
    request.data[4] = data_bits;
    request.data[5] = parity;
    request.data[6] = stop_bits;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_rs485_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_RS485) == 0U)
    {
        return 0U;
    }

    if ((data == NULL) || (length == 0U) || (length > 61U))
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_RS485_WRITE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_RS485_WRITE;
    request.sequence = sequence;
    request.length = length;
    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_rs485_read(uint8_t slot, uint8_t sequence)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_RS485) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_RS485_READ) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_RS485_READ;
    request.sequence = sequence;
    request.length = 0U;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_can_config(uint8_t slot, uint8_t sequence, uint32_t nominal_bitrate, uint32_t data_bitrate, uint8_t fd_enabled, uint8_t brs_enabled, uint8_t mode, uint8_t can_termination)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_CAN) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_CAN_CONFIG) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_CAN_CONFIG;
    request.sequence = sequence;
    request.length = 12U;

    request.data[0] = (uint8_t)(nominal_bitrate >> 24U);
    request.data[1] = (uint8_t)(nominal_bitrate >> 16U);
    request.data[2] = (uint8_t)(nominal_bitrate >> 8U);
    request.data[3] = (uint8_t)nominal_bitrate;

    request.data[4] = (uint8_t)(data_bitrate >> 24U);
    request.data[5] = (uint8_t)(data_bitrate >> 16U);
    request.data[6] = (uint8_t)(data_bitrate >> 8U);
    request.data[7] = (uint8_t)data_bitrate;

    request.data[8]  = fd_enabled;
    request.data[9]  = brs_enabled;
    request.data[10] = mode;
    request.data[11] = can_termination;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if ((fd_length == 0U) ||
        (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U))
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_can_std_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if ((data == NULL) || (length < 4U) || (length > 12U))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_CAN) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_CAN_STD_SEND) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_CAN_STD_SEND;
    request.sequence = sequence;
    request.length = (uint8_t)length;

    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if ((fd_length == 0U) || (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U))
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_can_fd_write( uint8_t slot, uint8_t sequence, const uint8_t *data, uint16_t length)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if ((data == NULL) || (length < 4U) || (length > 52U))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_CAN) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_CAN_FD_SEND) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_CAN_FD_SEND;
    request.sequence = sequence;
    request.length = (uint8_t)length;

    for (uint16_t i = 0U; i < length; i++)
    {
        request.data[i] = data[i];
    }

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if ((fd_length == 0U) || (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U))
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_can_read(uint8_t slot, uint8_t sequence)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (card_interface_ready(slot, CAP_CAN) == 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_CAN_RECEIVE) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    request.command = CAN_PROTOCOL_CMD_CAN_RECEIVE;
    request.sequence = sequence;
    request.length = 0U;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id =((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

uint8_t card_manager_multipart_test(uint8_t slot, uint8_t sequence)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return 0U;
    }

    if (transaction_multipart_response[slot].ready != 0U)
    {
        return 0U;
    }

    if (card_transaction_start(slot, sequence, CAN_PROTOCOL_CMD_MULTIPART_TEST) == 0U)
    {
        return 0U;
    }

    can_protocol_request_t request = {0};

    card_transaction_multipart_response_t *completed = &transaction_multipart_response[slot];
    completed->ready = 0U;
    completed->sequence = sequence;
    completed->command = CAN_PROTOCOL_CMD_MULTIPART_TEST;
    completed->status = 0U;
    completed->length = 0U;
    completed->count = 0U;
    completed->package = 0U;

    request.command = CAN_PROTOCOL_CMD_MULTIPART_TEST;
    request.sequence = sequence;
    request.length = 0U;

    uint8_t protocol_length = (uint8_t)(3U + request.length);
    uint8_t fd_length = can_fd_pad_length(protocol_length);
    uint16_t can_id = ((uint16_t)slot << 8U);

    if (can_send_std(can_id, (const uint8_t *)&request, fd_length) == 0U)
    {
        card_transaction_clear(slot);
        return 0U;
    }

    return 1U;
}

const card_transaction_response_t *card_manager_response_get(uint8_t slot)
{
    if ((slot == 0U) || (slot > slot_count))
    {
        return NULL;
    }

    if (transaction_response[slot].ready == 0U)
    {
        return NULL;
    }

    return &transaction_response[slot];
}

void card_manager_response_clear(uint8_t slot)
{
    if ((slot == 0U) || (slot > SLOT_COUNT_MAX))
    {
        return;
    }

    transaction_response[slot].ready = 0U;
    transaction_response[slot].sequence = 0U;
    transaction_response[slot].command = 0U;
    transaction_response[slot].status = 0U;
    transaction_response[slot].length = 0U;
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