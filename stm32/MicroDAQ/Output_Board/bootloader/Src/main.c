
#include <stdint.h>
#include "stm32c5xx.h"
#include "led.h"
#include "output.h"
#include "input.h"
#include "timer.h"
#include "can.h"

#define FLASH_START     0x08000000U
#define FLASH_END       0x0803FFFFU

#define APP_ADDR        0x08008000U

#define RAM_START       0x20000000U
#define RAM_END         0x20010000U 

#define FAULT_PIN 14U // PB14

typedef void (*func_ptr)(void);

uint32_t can_id;
can_id_type_t id_type;
uint8_t can_data[8];
uint8_t can_length;
can_id_type_t can_id_type;
can_state_t state;
uint8_t data_can1[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x00};
uint8_t data_can2[8] = {0x88, 0x99, 0x00, 0x11, 0x22, 0x33, 0x44, 0x55};

static uint8_t app_vectors_look_valid(void)
{
    uint32_t msp   = *(uint32_t *)APP_ADDR;
    uint32_t reset = *(uint32_t *)(APP_ADDR + 4U);

    if ((msp <= RAM_START) || (msp > RAM_END))
        return 0U;

    if ((msp & 0x7U) != 0U)
        return 0U;

    if ((reset & 0x1U) == 0U)
        return 0U;

    uint32_t reset_addr = reset & ~0x1U;

    if ((reset_addr < APP_ADDR) || (reset_addr > FLASH_END))
        return 0U;

    return 1U;
}

static uint8_t should_jump_to_app(void)
{
    if (!app_vectors_look_valid())
        return 0;

    return 1;
}

static void jump_to_app(void)
{
    uint32_t app_msp   = *(uint32_t *)APP_ADDR;
    uint32_t app_reset = *(uint32_t *)(APP_ADDR + 4U);
    func_ptr app_entry = (func_ptr)app_reset;

    __disable_irq();

    SysTick->CTRL = 0U;
    SysTick->LOAD = 0U;
    SysTick->VAL  = 0U;

    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk |
                SCB_ICSR_PENDSVCLR_Msk;

    for (uint32_t i = 0U; i < 8U; i++)
    {
        NVIC->ICER[i] = 0xFFFFFFFFU;
        NVIC->ICPR[i] = 0xFFFFFFFFU;
    }

    SCB->VTOR = APP_ADDR;

    __DSB();
    __ISB();

    __set_CONTROL(0U);
    __ISB();

    __set_MSP(app_msp);

    __DSB();
    __ISB();

    __enable_irq();

    app_entry();

    while (1){}
}

static void fault_pin(void)
{
    gpio_output_init(GPIOB, FAULT_PIN);
}

int main(void)
{
    if (should_jump_to_app())
        jump_to_app();

    input_init();
    slot_id_read();

    led_init();
    fault_pin();

    tim2_init();

    can_init();

	for(;;)
    {
        if (can_rx0_irq != 0U)
        {
            can_rx0_irq = 0U;
            can_receive_fifo0(&can_id, &can_id_type, can_data, &can_length);
        }

        if (sync_state)
        {
            sync_state = 0U;
        }

        if (clock_tick_flag)
        {
            clock_tick_flag = 0U;

            data_can1[7]++;
            can_send_std(can_id_make(slot_id, 0x01U), data_can1, 8U);
            can_send_std(can_id_make(slot_id, 0x02U), data_can2, 8U);
            if (data_can1[7] == 0xFFU)
            {
                data_can1[7] = 0U;
            }

            if(led2_state)
            {
                led2_state = 0U;
                led_off(GPIOA, LED2_PIN);
            }
            else
            {
                led2_state = 1U;
                led_on(GPIOA, LED2_PIN);
            }
        }
    };
}
