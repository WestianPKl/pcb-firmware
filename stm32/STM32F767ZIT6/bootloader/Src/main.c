#include "stm32f7xx.h"
#include <stdio.h>
#include "support.h"
#include "outputs.h"
#include "uart.h"
#include "dma.h"
#include "inputs.h"
#include "protocol.h"
#include "timers.h"

#define APP_ADDR        0x08008000U
#define RAM_START       0x20000000U
#define RAM_END         0x20080000U

typedef void (*func_ptr)(void);

void btn1_handler(void);
void uart2_handler(void);

static uint8_t app_vectors_look_valid(void)
{
    uint32_t msp   = *(uint32_t*)APP_ADDR;
    uint32_t reset = *(uint32_t*)(APP_ADDR + 4u);

    if (msp < RAM_START || msp > RAM_END) return 0;

    if (reset < APP_ADDR || reset > FLASH_END) return 0;

    return 1;
}

static uint8_t force_bootloader_button(void)
{
    return 0;
}

static uint8_t should_jump_to_app(void)
{
    if (!app_vectors_look_valid())
        return 0;

    if (force_bootloader_button())
        return 0;
    return 1;
}

static void jump_to_app(void)
{
    uint32_t app_msp   = *(uint32_t*)APP_ADDR;
    uint32_t app_reset = *(uint32_t*)(APP_ADDR + 4u);
    func_ptr app_entry = (func_ptr)(app_reset | 1u);

    __disable_irq();

    dma1_stop_all();

    if (RCC->APB1ENR & RCC_APB1ENR_USART2EN) {
        NVIC_DisableIRQ(USART2_IRQn);
        USART2->CR1 &= ~(USART_CR1_RXNEIE | USART_CR1_IDLEIE);
    }

    if (RCC->APB1ENR & RCC_APB1ENR_TIM2EN) {
        TIM2->CR1 &= ~TIM_CR1_CEN;
        TIM2->DIER = 0;
        TIM2->SR = 0;
    }

    if (RCC->AHB1ENR & RCC_AHB1ENR_GPIOBEN) {
        GPIOB->ODR &= ~((1U << LED1) | (1U << LED3));
    }

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

    __enable_irq();

    app_entry();
}

int main(void)
{
    if (should_jump_to_app())
        jump_to_app();

    setvbuf(stdout, NULL, _IONBF, 0);

    dwt_init();
    init_leds();
    init_btn();
    init_timer_tim2_ch1();

    dma1_init();
    uart3_rxtx_init();
    uart2_rxtx_init();
    uart3_dma1_rx_start(uart3_data, uart3_buffer_size);
    uart2_dma1_rx_start(uart2_data, uart2_buffer_size);

    for(;;) {

        if (btn1_flag) {
            btn1_flag = 0;
            btn1_handler();
        }

        if (uart2_rx_flag) {
            uart2_rx_flag = 0;
            uart2_handler();
        }

        if (clock_tick_flag) {
            clock_tick_flag = 0;
            GPIOB->ODR ^= (1U << LED1);
        }
    }
}

void btn1_handler(void)
{
    delay_ms(10);
    if (GPIOC->IDR & BTN_1) return;

    GPIOB->ODR ^= (1U << LED3);
}

void uart2_handler(void)
{
    uint16_t n = uart2_rx_len;
    if (n > uart2_buffer_size) n = uart2_buffer_size;

    uint8_t valid = (n >= 7U)
        && (uart2_data[0] == MAGIC)
        && (uart2_data[1] <= PROTOCOL_MAX_PAYLOAD)
        && (n == (uint16_t)(uart2_data[1] + 7U))
        && (uart2_data[2] == ADDRESS);

    if (valid) {
        uint16_t crc = crc16_ccitt(&uart2_data[1], 4U + uart2_data[1]);
        valid = (crc == (uint16_t)((uart2_data[5 + uart2_data[1]] << 8) | uart2_data[6 + uart2_data[1]]));
    }

    protocol_request_t c;
    if (valid) {
        c.magic = uart2_data[0];
        c.length = uart2_data[1];
        c.address = uart2_data[2];
        c.sequence = uart2_data[3];
        c.command = uart2_data[4];
        for (uint16_t i = 0; i < c.length; i++) {
            c.data[i] = uart2_data[5 + i];
        }
    }

    uart2_dma1_rx_start(uart2_data, uart2_buffer_size);

    static uint8_t frame[PROTOCOL_FRAME_MAX];

    if (valid) {
        uint32_t t = 1000000U;
        while (uart2_tx_busy) {
            if (--t == 0U) {
                uart2_tx_busy = 0;
                break;
            }
        }

        uint16_t len = protocol_process_request(&c, frame);
        uart2_write_dma(frame, len);
    }
}

