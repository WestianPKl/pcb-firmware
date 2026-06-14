#include <stdint.h>
#include <stdbool.h>
#include <string.h>
#include <stdio.h>
#include "systick.h"
#include "uart.h"
#include "version.h"

#define APP_ADDR        0x08008000U
#define INFO_ADDR       0x080FF800U

#define DEV_ADDR        0xB2
#define FRAME_LEN       64
#define PAYLOAD_MAX     60

#define STATUS_OK       0x40
#define STATUS_ERR      0x7F

typedef void (*func_ptr)(void);

static void bootloader_init(void){
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(3U << (14U * 2U));
    GPIOB->MODER |=  (1U << (14U * 2U));
    GPIOB->OTYPER &= ~(1U << 14U);
    GPIOB->OSPEEDR |= (2U << (14U * 2U));
    GPIOB->PUPDR &= ~(3U << (14U * 2U));

    uart2_rxtx_init();
    systick_delay_ms(100);
}

static bool app_vectors_look_valid(void)
{
    uint32_t msp   = *(uint32_t*)APP_ADDR;
    uint32_t reset = *(uint32_t*)(APP_ADDR + 4u);

    if (msp < 0x20000000u || msp > 0x20040000u) return false;

    if (reset < APP_ADDR || reset >= 0x08100000u) return false;

    return true;
}

static bool force_bootloader_button(void)
{
    return false;
}

static bool should_jump_to_app(void)
{
    if (!app_vectors_look_valid())
        return false;

    if (force_bootloader_button())
        return false;
    return true;
}

static void jump_to_app(void)
{
    uint32_t app_msp   = *(uint32_t*)APP_ADDR;
    uint32_t app_reset = *(uint32_t*)(APP_ADDR + 4u);
    func_ptr app_entry = (func_ptr)(app_reset | 1u);

    GPIOB->ODR &= ~(1U << 14U);

    __disable_irq();

    NVIC_DisableIRQ(USART2_IRQn);
    USART2->CR1 &= ~USART_CR1_RXNEIE;

    SysTick->CTRL = 0;
    SysTick->LOAD = 0;
    SysTick->VAL  = 0;

    SCB->ICSR = SCB_ICSR_PENDSTCLR_Msk | SCB_ICSR_PENDSVCLR_Msk;

    for (uint32_t i = 0; i < 8; i++) {
        NVIC->ICER[i] = 0xFFFFFFFFu;
        NVIC->ICPR[i] = 0xFFFFFFFFu;
    }

    SCB->VTOR = APP_ADDR;
    __DSB(); __ISB();

    __set_MSP(app_msp);
    __DSB(); __ISB();

    app_entry();
}

static bool uart_read_frame(uint8_t *frame)
{
    static uint8_t tmp[FRAME_LEN];
    static uint8_t idx = 0;
    uint8_t b;
    while (uart2_rx_pop(&b))
    {
        if (idx == 0)
        {
            if (b != 0xB2) continue;
            tmp[idx++] = b;
        }
        else
        {
            tmp[idx++] = b;
            if (idx == FRAME_LEN)
            {
                uint8_t calc = crc8_atm(tmp, FRAME_LEN - 1);
                uint8_t rxcrc = tmp[FRAME_LEN - 1];
                idx = 0;

                if (calc != rxcrc) return false;

                memcpy(frame, tmp, FRAME_LEN);
                return true;
            }
        }
    }
    return false;
}

static void send_response(uint8_t status, uint8_t cmd, const uint8_t *payload, uint8_t len)
{
    uint8_t resp[FRAME_LEN] = {DEV_ADDR, status, cmd, 0};

    if (payload && len) {
        if (len > PAYLOAD_MAX) len = PAYLOAD_MAX;
        memcpy(&resp[4], payload, len);
    }

    resp[FRAME_LEN - 1] = crc8_atm(resp, FRAME_LEN - 1);
    uart2_send(resp, FRAME_LEN);
}

static void bootloader_loop(void)
{
    while (1)
    {

        uint8_t req[64];
        if (!uart_read_frame(req)) continue;

        uint8_t addr       = req[0];
        uint8_t cmd        = req[2];
        uint8_t param_addr = req[3];
        uint8_t param      = req[4];

        (void)addr; (void)param_addr; (void)param;
        switch (cmd)
        {
            case 0x00: {
                uint8_t ver[3] = {0xFF, 0xFF, 0xFF};
                send_response(STATUS_OK, cmd, ver, sizeof(ver));
                GPIOB->ODR ^= (1U << 14U);
                break;
            }

            case 0x10: {
                uint8_t ver[3] = {BL_VERSION_MAJOR, BL_VERSION_MINOR, BL_VERSION_PATCH};
                send_response(STATUS_OK, cmd, ver, sizeof(ver));
                GPIOB->ODR ^= (1U << 14U);
                break;
            }

            case 0x11: {
                send_response(STATUS_OK, cmd, (const uint8_t*)BL_BUILD_DATE, strlen(BL_BUILD_DATE));
                GPIOB->ODR ^= (1U << 14U);
                break;
            }

            case 0x20: {
                send_response(STATUS_OK, cmd, (const uint8_t*)INFO_ADDR, sizeof(device_info_t));
                GPIOB->ODR ^= (1U << 14U);
                break;
            }

            default:
                send_response(STATUS_ERR, cmd, NULL, 0);
                GPIOB->ODR ^= (1U << 14U);
                break;
        }
    }
}

static void blink_start(uint8_t times, uint16_t delay_ms)
{
    for (uint8_t i = 0; i < times; i++)
    {
        GPIOB->ODR |= (1U << 14U);
        systick_delay_ms(delay_ms);
        GPIOB->ODR &= ~(1U << 14U);
        systick_delay_ms(delay_ms);
    }
}


int main(void)
{
    bootloader_init();
    blink_start(10, 50);

    if (should_jump_to_app())
        jump_to_app();

    bootloader_loop();
}