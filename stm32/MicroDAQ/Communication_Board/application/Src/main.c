
#include <stdint.h>
#include "stm32c5xx.h"
#include "led.h"
#include "output.h"
#include "input.h"
#include "timer.h"
#include "can.h"
#include "protocol.h"
#include "support.h"
#include "fault.h"
#include "sync.h"
#include "version.h"
#include "i2c.h"
#include "spi.h"
#include "uart.h"
#include "rs232.h"
#include "rs485.h"

uint32_t can_id;
can_id_type_t id_type;
uint8_t can_data[64];
uint8_t can_length;
can_id_type_t can_id_type;
can_state_t state;

uint32_t can2_id;
can_id_type_t can2_id_type;
uint8_t fd;
uint8_t brs;
uint8_t can2_data[64];
uint8_t can2_length;
can_state_t can2_state;

static reset_reason_t reset_reason = {0};

static void system_fault_process(void)
{
    can_state_t state = can_get_state();
    can_state_t can2_state = can2_get_state();

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

    if (can2_state == CAN_STATE_BUS_OFF)
        fault_set(FAULT_CAN2_BUS_OFF);
    else
        fault_clear(FAULT_CAN2_BUS_OFF);

    if (can2_state == CAN_STATE_PASSIVE)
        fault_set(FAULT_CAN2_ERROR_PASSIVE);
    else
        fault_clear(FAULT_CAN2_ERROR_PASSIVE);

    if (can2_state == CAN_STATE_WARNING)
        fault_set(FAULT_CAN2_ERROR_WARNING);
    else
        fault_clear(FAULT_CAN2_ERROR_WARNING);
}

int main(void)
{
    reset_reason_get(&reset_reason);
    protocol_service_init(&reset_reason);

    input_init();
    slot_id_read();

    led_init();
    fault_pin_init();
    sync_init();

    tim2_init();
    i2c1_init();
    spi1_init();
    uart5_init(115200);
    rs232_init(115200);
    rs485_init(115200);

    iwdg_init();
    can_init();
    can2_init();

    if (board_info_valid() == 0U)
    {
        fault_set(FAULT_BOARD_INFO);
    }

    gpio_output_init(GPIOC, CAN_TERM_PIN);
    gpio_output_init(GPIOC, RS485_TERM_PIN);

    output_off(GPIOC, CAN_TERM_PIN);
    output_off(GPIOC, RS485_TERM_PIN);

    led_on(GPIOB, LED1_PIN);
    led_off(GPIOB, LED2_PIN);
    led_off(GPIOA, LED3_PIN);
    led_off(GPIOA, LED4_PIN); 

	for(;;)
    {
        uart5_process();
        rs232_process();
        rs485_process();
        
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

        if (can2_bo_irq != 0U)
        {
            can2_bo_irq = 0U;

            fault_set(FAULT_CAN2_BUS_OFF);
            can2_recovery_start();
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

        if (can2_rx0_irq != 0U)
        {
            can2_rx0_irq = 0U;

            while (can2_receive_fifo0(&can2_id, &can2_id_type, can2_data, &can2_length, &fd, &brs) != 0U)
            {
                can2_rx_callback(can2_id, can2_id_type, can2_data, can2_length, fd, brs);
            }
        }

        if ((can2_tx_irq != 0U) || (can2_tx_fail_irq != 0U))
        {
            can2_tx_irq = 0U;
            can2_tx_fail_irq = 0U;

            if (can2_tx_buffer_index < 3U)
            {
                uint32_t mask = 1UL << can2_tx_buffer_index;

                if ((FDCAN2->TXBTO & mask) != 0U)
                {
                    can2_tx_buffer_index = 0xFFU;
                    can2_tx_callback(1U);
                }
                else if ((FDCAN2->TXBCF & mask) != 0U)
                {
                    can2_tx_buffer_index = 0xFFU;
                    can2_tx_callback(0U);
                }
            }
        }
        can2_tx_process();
        multipart_tx_process();

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
                led_on(GPIOB, LED2_PIN);
            }
            else
            {
                led_off(GPIOB, LED2_PIN);
            }
        }
    };
}
