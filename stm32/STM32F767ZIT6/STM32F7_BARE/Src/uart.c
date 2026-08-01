#include "uart.h"
#include "dma.h"
#include "main.h"
#include "support.h"

uint8_t uart3_data[uart3_buffer_size];
uint8_t uart2_data[uart2_buffer_size];

static uint16_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

void uart_write(USART_TypeDef *USARTx, uint8_t data)
{
    while (!(USARTx->ISR & USART_ISR_TXE)){};
    USARTx->TDR = data;
}

void uart_read(USART_TypeDef *USARTx, uint8_t *data)
{
    while (!(USARTx->ISR & USART_ISR_RXNE)){};
    *data = (uint8_t)(USARTx->RDR & 0xFF);
}

void uart3_rxtx_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(0x3U << (UART3_TX * 2U));
    GPIOD->MODER |=  (0x2U << (UART3_TX * 2U));
    GPIOD->AFR[1] &= ~(0xFU << (0U * 4U));
    GPIOD->AFR[1] |=  (0x7U << (0U * 4U));

    GPIOD->MODER &= ~(0x3U << (UART3_RX * 2U));
    GPIOD->MODER |=  (0x2U << (UART3_RX * 2U));
    GPIOD->AFR[1] &= ~(0xFU << (1U * 4U));
    GPIOD->AFR[1] |=  (0x7U << (1U * 4U));

    RCC->APB1ENR |= RCC_APB1ENR_USART3EN;
    (void)RCC->APB1ENR;

    USART3->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);
    USART3->CR2 &= ~USART_CR2_STOP;

    USART3->CR1 |= USART_CR1_TE | USART_CR1_RE;

    uint32_t pclk = apb1_pclk_hz();
    USART3->BRR = compute_uart_div(pclk, 115200);

    USART3->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;

    USART3->CR1 |= USART_CR1_UE;
}


#define UART3_TX_BUF_SIZE 256U

static uint8_t uart3_tx_dma_buf[UART3_TX_BUF_SIZE];

static void uart3_wait_tx_idle(void)
{
    uint32_t t = 1000000U;
    while (uart3_tx_busy) {
        if (--t == 0U) {
            uart3_tx_busy = 0;
            break;
        }
    }
}

void uart2_rxtx_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(0x3U << (UART2_TX * 2U));
    GPIOD->MODER |=  (0x2U << (UART2_TX * 2U));
    GPIOD->AFR[0] &= ~(0xFU << (5U * 4U));
    GPIOD->AFR[0] |=  (0x7U << (5U * 4U));

    GPIOD->MODER &= ~(0x3U << (UART2_RX * 2U));
    GPIOD->MODER |=  (0x2U << (UART2_RX * 2U));
    GPIOD->AFR[0] &= ~(0xFU << (6U * 4U));
    GPIOD->AFR[0] |=  (0x7U << (6U * 4U));
    GPIOD->PUPDR &= ~(0x3U << (UART2_RX * 2U));
    GPIOD->PUPDR |=  (0x1U << (UART2_RX * 2U));

    RCC->APB1ENR |= RCC_APB1ENR_USART2EN;
    (void)RCC->APB1ENR;

    USART2->CR1 &= ~(USART_CR1_M | USART_CR1_PCE | USART_CR1_PS);
    USART2->CR2 &= ~USART_CR2_STOP;

    USART2->CR1 |= USART_CR1_TE | USART_CR1_RE;

    uint32_t pclk = apb1_pclk_hz();
    // USART2->BRR = compute_uart_div(pclk, 256000);
    USART2->BRR = compute_uart_div(pclk, 115200);

    USART2->CR3 |= USART_CR3_DMAR | USART_CR3_DMAT;

    USART2->CR1 |= USART_CR1_IDLEIE;
    NVIC_SetPriority(USART2_IRQn, 5);
    NVIC_EnableIRQ(USART2_IRQn);
    NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    USART2->CR1 |= USART_CR1_UE;
}


#define UART2_TX_BUF_SIZE 256U

int _write(int fd, const char *buf, int len)
{   
    xSemaphoreTake(printf_mutex, portMAX_DELAY);
    (void)fd;
    if (len <= 0) {
        xSemaphoreGive(printf_mutex);
        return 0;
    };

    int sent = 0;
    while (sent < len) {
        uint16_t chunk = (uint16_t)((len - sent) > (int)UART3_TX_BUF_SIZE
                                    ? UART3_TX_BUF_SIZE
                                    : (uint16_t)(len - sent));
        uart3_wait_tx_idle();
        for (uint16_t i = 0; i < chunk; i++) {
            uart3_tx_dma_buf[i] = (uint8_t)buf[sent + i];
        }
        uart3_write_dma(uart3_tx_dma_buf, chunk);
        sent += chunk;
    }
    xSemaphoreGive(printf_mutex);
    return len;
}

int __io_putchar(int ch)
{
    uint8_t c = (uint8_t)ch;
    _write(1, (const char *)&c, 1);
    return ch;
}

volatile uint16_t uart2_rx_len = 0;

void USART2_IRQHandler(void)
{
    if (USART2->ISR & (USART_ISR_ORE | USART_ISR_FE | USART_ISR_NE)) {
        USART2->ICR = USART_ICR_ORECF | USART_ICR_FECF | USART_ICR_NCF;
    }

    if (USART2->ISR & USART_ISR_IDLE) {
        USART2->ICR = USART_ICR_IDLECF;

        uart2_rx_len = uart2_buffer_size - (uint16_t)DMA1_Stream5->NDTR;
        BaseType_t xHigherPriorityTaskWoken = pdFALSE;
        vTaskNotifyGiveFromISR(task_uart2_handle, &xHigherPriorityTaskWoken);
        portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
    }
}