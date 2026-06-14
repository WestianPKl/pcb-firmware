#include "uart.h"

// PA2: USART2_TX
// PA3: USART2_RX

// PA9: USART1_TX
// PA10: USART1_RX


#define PA2 2U
#define PA3 3U
#define PA9 9U
#define PA10 10U
#define UART_BAUDRATE 115200U

static uint32_t get_pclk1_hz(void)
{
    uint32_t hclk = SystemCoreClock;
    // uint32_t ppre1 = (RCC->CFGR >> 8) & 0x7;
    uint32_t ppre1 = (RCC->CFGR >> 10) & 0x7;
    uint32_t div = 1;
    if (ppre1 >= 4) {
        div = 1U << (ppre1 - 3U);
    }
    return hclk / div;
}

static uint32_t get_pclk2_hz(void)
{
    uint32_t hclk = SystemCoreClock;
    // uint32_t ppre2 = (RCC->CFGR >> 11) & 0x7;
    uint32_t ppre2 = (RCC->CFGR >> 13) & 0x7;
    uint32_t div = 1;
    if (ppre2 >= 4) {
        div = 1U << (ppre2 - 3U);
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
    uart_set_baudrate(USART2, pclk1, UART_BAUDRATE);

    USART2->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;

    USART2->CR1 = 0;
    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;

    USART2->SR &= ~USART_SR_TC;

    USART2->CR1 |= USART_CR1_UE;
}

void uart1_rxtx_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~((3U << (PA9 * 2U)) | (3U << (PA10 * 2U)));
    GPIOA->MODER |=  (2U << (PA9 * 2U)) | (2U << (PA10 * 2U));

    GPIOA->PUPDR &= ~((3U << (PA9 * 2U)) | (3U << (PA10 * 2U)));
    GPIOA->OSPEEDR |= (2U << (PA9 * 2U)) | (2U << (PA10 * 2U));
    
    GPIOA->AFR[1] &= ~((0xFU << ((9U-8U)*4U)) | (0xFU << ((10U-8U)*4U)));
    GPIOA->AFR[1] |=  ((7U   << ((9U-8U)*4U)) | (7U   << ((10U-8U)*4U)));

    RCC->APB2ENR |= RCC_APB2ENR_USART1EN;
    (void)RCC->APB2ENR;

    uint32_t pclk2 = get_pclk2_hz();
    uart_set_baudrate(USART1, pclk2, UART_BAUDRATE);

    USART1->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;

    USART1->CR1 = 0;
    USART1->CR1 |= USART_CR1_TE | USART_CR1_RE;

    USART1->SR &= ~USART_SR_TC;

    USART1->CR1 |= USART_CR1_UE;
}