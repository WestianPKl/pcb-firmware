#include "uart.h"

#define PA2 2U
#define PA3 3U
#define PA9 9U
#define PA10 10U
#define UART2_BAUDRATE 115200U

#define RX_BUF_SIZE 256U
static volatile uint8_t  rx_buf[RX_BUF_SIZE];
static volatile uint16_t rx_head = 0;
static volatile uint16_t rx_tail = 0;


// PA2: USART2_TX
// PA3: USART2_RX

// PA9: USART1_TX
// PA10: USART1_RX

static inline void rx_push(uint8_t byte)
{
    uint16_t next = (rx_head + 1U) % RX_BUF_SIZE;
    if (next != rx_tail) {
        rx_buf[rx_head] = byte;
        rx_head = next;
    }
}

bool uart2_rx_pop(uint8_t *out)
{
    if (rx_tail == rx_head) return false;
    *out = rx_buf[rx_tail];
    rx_tail = (rx_tail + 1U) % RX_BUF_SIZE;
    return true;
}

static uint32_t get_pclk1_hz(void)
{
    uint32_t hclk = SystemCoreClock;
    uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7;
    uint32_t div = 1;
    if (ppre1 >= 4) { 
        div = 1U << (ppre1 - 3U);
    } 
    return hclk / div;
}

static uint32_t compute_uart_div(uint32_t clk, uint32_t baud)
{
    return (clk + (baud / 2U)) / baud;
}

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t pclk, uint32_t baud)
{
    USARTx->BRR = compute_uart_div(pclk, baud);
}

void uart2_rxtx_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~((3U << (PA2 * 2U)) | (3U << (PA3 * 2U)));
    GPIOA->MODER |=  (2U << (PA2 * 2U)) | (2U << (PA3 * 2U));

    GPIOA->PUPDR &= ~((3U << (PA2 * 2U)) | (3U << (PA3 * 2U)));
    GPIOA->OSPEEDR |= (2U << (PA2 * 2U)) | (2U << (PA3 * 2U));

    GPIOA->AFR[0] &= ~((0xFU << (PA2 * 4U)) | (0xFU << (PA3 * 4U)));
    GPIOA->AFR[0] |=  ((7U   << (PA2 * 4U)) | (7U   << (PA3 * 4U)));

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    uint32_t pclk1 = get_pclk1_hz();
    uart_set_baudrate(USART2, pclk1, UART2_BAUDRATE);

    USART2->CR1 = 0;
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;

    USART2->CR1 |= USART_CR1_UE;

    USART2->CR1 |= USART_CR1_RXNEIE;

    NVIC_EnableIRQ(USART2_IRQn);
}

void uart2_write(uint8_t ch)
{
    while (!(USART2->SR & USART_SR_TXE)) {}
    USART2->DR = ch;
}

void uart2_send(const uint8_t *buf, uint32_t len)
{
    for (uint32_t i = 0; i < len; i++) {
        uart2_write(buf[i]);
    }
    while (!(USART2->SR & USART_SR_TC)) {}
}

uint8_t crc8_atm(const uint8_t *data, uint32_t len)
{
    uint8_t crc = 0x00;

    for (uint32_t i = 0; i < len; i++) {
        crc ^= data[i];
        for (uint8_t b = 0; b < 8; b++) {
            if (crc & 0x80)
                crc = (crc << 1) ^ 0x07;
            else
                crc <<= 1;
        }
    }
    return crc;
}

void USART2_IRQHandler(void)
{
    if (USART2->SR & USART_SR_RXNE) {
        rx_push((uint8_t)USART2->DR);
    }
}
