
#include <stdint.h>
#include "stm32c5xx.h"
#include "led.h"
#include "digital_output.h"
#include "pwm.h"
#include "output.h"
#include "input.h"
#include "timer.h"
#include "can.h"
#include "protocol.h"
#include "support.h"
#include "fault.h"
#include "sync.h"
#include "version.h"

uint32_t can_id;
can_id_type_t id_type;
uint8_t can_data[64];
uint8_t can_length;
can_id_type_t can_id_type;
can_state_t state;

static reset_reason_t reset_reason = {0};

static void system_fault_process(void)
{
    can_state_t state = can_get_state();

    if (state == CAN_STATE_BUS_OFF)
        fault_set(FAULT_CAN_BUS_OFF);
    else
        fault_clear(FAULT_CAN_BUS_OFF);

    if (state == CAN_STATE_PASSIVE)
        fault_set(FAULT_CAN_ERROR_PASSIVE);
    else
        fault_clear(FAULT_CAN_ERROR_PASSIVE);

    if (state == CAN_STATE_WARNING)
        fault_set(FAULT_CAN_ERROR_WARNING);
    else
        fault_clear(FAULT_CAN_ERROR_WARNING);

    if (digital_output_get_mask() != digital_output_get_gpio_mask())
        fault_set(FAULT_DIGITAL_OUTPUT);
    else
        fault_clear(FAULT_DIGITAL_OUTPUT);
}

int main(void)
{
    reset_reason_get(&reset_reason);
    protocol_service_init(&reset_reason);

    input_init();
    slot_id_read();

    led_init();
    digital_output_init();
    pwm_init();
    fault_pin_init();
    sync_init();

    tim6_init();

    iwdg_init();
    can_init();

    if (board_info_valid() == 0U)
    {
        fault_set(FAULT_BOARD_INFO);
    }

    led_on(GPIOA, LED1_PIN);
    led_off(GPIOA, LED2_PIN);
    led_off(GPIOA, LED3_PIN);
    led_off(GPIOA, LED4_PIN); 

	for(;;)
    {
        if (sync_event != 0U)
        {
            sync_event = 0U;
        }

        if (can_bo_irq != 0U)
        {
            can_bo_irq = 0U;

            fault_set(FAULT_CAN_BUS_OFF);
            can_recovery_start();
        }

        if (can_rx0_irq != 0U)
        {
            can_rx0_irq = 0U;
            while (can_receive_fifo0(&can_id, &can_id_type, can_data, &can_length) != 0U)
            {
                if ((can_id_type == CAN_ID_STD) && (can_id == can_id_make(slot_id, 0x00U)))
                {
                    can_process_response((const can_protocol_request_t *)can_data, can_length);
                }
            }
        }

        if (clock_tick_100ms_flag)
        {
            clock_tick_100ms_flag = 0U;
            comm_led_process_100ms();
            can_send_100ms_frames();
            identify_process();
        }

        if (clock_tick_flag)
        {
            clock_tick_flag = 0U;
            iwdg_refresh();

            system_fault_process();

            can_send_1s_frames();

            if (fault_active() != 0U)
            {
                led_on(GPIOA, LED2_PIN);
            }
            else
            {
                led_off(GPIOA, LED2_PIN);
            }
        }
    };
}
