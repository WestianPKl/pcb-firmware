#include "uart.h"
#include "support.h"

volatile uint8_t uart1_tx_busy;
uint8_t uart1_data[uart1_buffer_size];
volatile uint16_t uart1_rx_length = 0U;
volatile uint8_t uart1_rx_ready = 0U;

static uint32_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

void uart1_dma1_tx_start(uint8_t *dst, uint16_t len)
{
    DMA1_Channel3->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos));
    while ((DMA1_Channel3->CCR & (0x1U << DMA_CCR_EN_Pos)) != 0U) {}

    DMA1_Channel3->CPAR = (uint32_t)&USART1->TDR;
    DMA1_Channel3->CMAR = (uint32_t)dst;
    DMA1_Channel3->CNDTR = len;

    DMA1_Channel3->CCR = 0;
    DMA1_Channel3->CCR |=
        (0x1U << DMA_CCR_PL_Pos)   |
        DMA_CCR_MINC_Msk           |
        DMA_CCR_DIR_Msk            |
        DMA_CCR_TCIE_Msk;

    DMA1->IFCR = 0xFU << (4U * (3U - 1U));

    NVIC_ClearPendingIRQ(  DMA1_Channel2_3_IRQn );
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 2U);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    DMA1_Channel3->CCR |= (0x1U << DMA_CCR_EN_Pos);

    USART1->CR3 |= (0x1U << USART_CR3_DMAT_Pos);
}

void uart1_dma1_rx_start(uint8_t *dst, uint16_t len)
{
    DMA1_Channel2->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos));
    while ((DMA1_Channel2->CCR & (0x1U << DMA_CCR_EN_Pos)) != 0U) {}

    DMA1_Channel2->CPAR = (uint32_t)&USART1->RDR;
    DMA1_Channel2->CMAR = (uint32_t)dst;
    DMA1_Channel2->CNDTR = len;

    DMA1_Channel2->CCR = 0;
    DMA1_Channel2->CCR |=
        (0x1U << DMA_CCR_PL_Pos) |
        DMA_CCR_MINC_Msk         |
        DMA_CCR_TCIE_Msk;

    DMA1->IFCR = 0xFU << (4U * (2U - 1U));

    NVIC_ClearPendingIRQ(DMA1_Channel2_3_IRQn);
    NVIC_SetPriority(DMA1_Channel2_3_IRQn, 2U);
    NVIC_EnableIRQ(DMA1_Channel2_3_IRQn);

    DMA1_Channel2->CCR |= (0x1U << DMA_CCR_EN_Pos);
}

uint8_t uart1_write_dma(uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return 0;

    if (uart1_tx_busy) {
        return 0;
    }

    uart1_tx_busy = 1;
    uart1_dma1_tx_start(data, len);
    return 1;
}

void uart1_receive_dma(uint8_t *dst, uint16_t len)
{
    if (!dst || len == 0) return;

    uart1_dma1_rx_start(dst, len);
}

void uart1_init(uint32_t baudrate)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    (void)RCC->IOPENR;

    GPIOA->MODER &= ~(0x3U << (UART1_TX_PIN * 2U) | 0x3U << (UART1_RX_PIN * 2U));
    GPIOA->MODER |= (0x2U << (UART1_TX_PIN * 2U) | 0x2U << (UART1_RX_PIN * 2U));

    GPIOA->OTYPER &= ~(0x1U << UART1_TX_PIN | 0x1U << UART1_RX_PIN);
    GPIOA->PUPDR &= ~(0x3U << (UART1_TX_PIN * 2U) | 0x3U << (UART1_RX_PIN * 2U));
    GPIOA->OSPEEDR &= ~(0x3U << (UART1_TX_PIN * 2U) | 0x3U << (UART1_RX_PIN * 2U));

    GPIOA->AFR[UART1_TX_PIN >> 3U] &= ~(0xFU << ((UART1_TX_PIN & 7U) * 4U));
    GPIOA->AFR[UART1_RX_PIN >> 3U] &= ~(0xFU << ((UART1_RX_PIN & 7U) * 4U));
    GPIOA->AFR[UART1_TX_PIN >> 3U] |= (0x1U << ((UART1_TX_PIN & 7U) * 4U));
    GPIOA->AFR[UART1_RX_PIN >> 3U] |= (0x1U << ((UART1_RX_PIN & 7U) * 4U));

    RCC->APBENR2 |= RCC_APBENR2_USART1EN;
    (void)RCC->APBENR2;

    USART1->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    USART1->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    USART1->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    uint32_t pclk = apb_pclk_hz();

    USART1->PRESC = 0x0U;

    USART1->BRR = compute_uart_div(pclk, baudrate);

    USART1->CR1 &= ~(USART_CR1_RXNEIE_RXFNEIE_Msk | USART_CR1_TXEIE_TXFNFIE_Msk);

    USART1->CR1 |= (0x1U << USART_CR1_RE_Pos) | (0x1U << USART_CR1_TE_Pos);

    USART1->CR3 |= (0x1U << USART_CR3_DMAR_Pos) | (0x1U << USART_CR3_DMAT_Pos);

    USART1->CR1 |= USART_CR1_IDLEIE;

    NVIC_ClearPendingIRQ(USART1_IRQn);
    NVIC_SetPriority(USART1_IRQn, 2U);
    NVIC_EnableIRQ(USART1_IRQn);

    USART1->CR1 |= USART_CR1_UE;

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);
    while ((USART1->ISR & ack_mask) != ack_mask) {}
}

void USART1_IRQHandler(void)
{
    if ((USART1->ISR & USART_ISR_IDLE) != 0U)
    {
        USART1->ICR = USART_ICR_IDLECF;

        DMA1_Channel2->CCR &= ~DMA_CCR_EN_Msk;

        uart1_rx_length =
            uart1_buffer_size - DMA1_Channel2->CNDTR;

        uart1_rx_ready = 1U;
    }
}

void DMA1_Channel2_3_IRQHandler(void)
{
    uint32_t status = DMA1->ISR;

    if ((status & DMA_ISR_TCIF2) != 0U)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF2;
        DMA1_Channel2->CCR &= ~DMA_CCR_EN_Msk;

        if (uart1_rx_ready == 0U)
        {
            uart1_rx_length = uart1_buffer_size;
            uart1_rx_ready = 1U;
        }
    }

    if ((status & DMA_ISR_TCIF3) != 0U)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF3;
        uart1_tx_busy = 0U;
    }
}