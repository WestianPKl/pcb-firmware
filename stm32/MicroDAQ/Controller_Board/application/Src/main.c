#include <stdint.h>
#include "stm32h5xx.h"
#include "support.h"
#include "output.h"
#include "input.h"
#include "timer.h"
#include "uart.h"
#include "protocol.h"
#include "can.h"
#include "ethernet.h"
#include "systick.h"
#include "card_manager.h"
#include "ethernetif.h"
#include "controller_tcp.h"
#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"
#include "version.h"
#include "fault.h"

static struct netif gnetif;
static uint8_t power_poll = 0U;
volatile uint8_t ethernet_state = 0U;

uint32_t can_id;
can_id_type_t can_id_type;
uint8_t can_data[64];
uint8_t can_length;

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
}

int main(void)
{
    reset_reason_get(&reset_reason);
    protocol_service_init(&reset_reason);

    clock_init_250mhz();
    systick_init(1000U);

    gpio_output_init(GPIOB, GREEN_LED_PIN);
    gpio_output_init(GPIOF, YELLOW_LED_PIN);
    gpio_output_init(GPIOG, RED_LED_PIN);

    card_manager_init();
    tim2_init();

    uart2_init(115200U);

    ethernet_state = ethernet_init();

    if (ethernet_state != 0U)
    {
        lwip_init();

        ip4_addr_t ipaddr;
        ip4_addr_t netmask;
        ip4_addr_t gateway;

        IP4_ADDR(&ipaddr,  192, 168, 18, 200);
        IP4_ADDR(&netmask, 255, 255, 255, 0);
        IP4_ADDR(&gateway, 192, 168, 18, 1);

        if (netif_add(&gnetif, &ipaddr, &netmask, &gateway, NULL, ethernetif_init, ethernet_input) != NULL)
        {
            netif_set_default(&gnetif);
            netif_set_up(&gnetif);
            netif_set_link_up(&gnetif);

            if (controller_tcp_init() == 0U)
            {
                ethernet_state = 0U;
            }
        }
        else
        {
            ethernet_state = 0U;
        }
    }

    iwdg_init();
    can_init();

    if (board_info_valid() == 0U)
    {
        fault_set(FAULT_BOARD_INFO);
    }

    card_manager_uart_send(UART_CMD_BOARD_INFO);

    for (;;)
    {
        uart2_process();

        if (ethernet_state != 0U)
        {
            ethernetif_input(&gnetif);
            sys_check_timeouts();
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
                if (can_id_type == CAN_ID_STD)
                {
                    (void)card_manager_can_receive(can_id, can_data, can_length);
                }
            }
        }

        if (can_rx1_irq != 0U)
        {
            can_rx1_irq = 0U;

            while (can_receive_fifo1(&can_id, &can_id_type, can_data, &can_length) != 0U)
            {
                if (can_id_type == CAN_ID_STD)
                {
                    (void)card_manager_can_receive(can_id, can_data, can_length);
                }
            }
        }

        card_manager_process();
        protocol_process();

        if (clock_tick_flag != 0U)
        {
            clock_tick_flag = 0U;
            iwdg_refresh();

            service_uptime++;

            card_manager_1s_process();
            system_fault_process();

            if (power_card.identified == 0U)
            {
                card_manager_uart_send(UART_CMD_BOARD_INFO);
            }
            else
            {
                uint8_t command = (power_poll == 0U) ? UART_CMD_ADC_DATA : UART_CMD_INA_DATA;
                if (card_manager_uart_send(command) != 0U)
                {
                    power_poll ^= 1U;
                }
            }
        }
    }
}
