#include "uart.h"
#include <stddef.h>
#include <string.h>

static uint8_t uart5_tx_buffer[UART5_TX_BUFFER_SIZE];
static volatile uint8_t uart5_rx_dma_buffer[UART5_RX_BUFFER_SIZE];

static volatile uint8_t uart5_rx_event = 0U;
static volatile uint8_t uart5_tx_event = 0U;

static volatile uint8_t uart5_rx_dma_error = 0U;
static volatile uint8_t uart5_tx_dma_error = 0U;

static uint32_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

static void uart5_tx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;
    (void)RCC->AHB1ENR;

    LPDMA1_CH0->CCR &= ~(DMA_CCR_EN_Msk | DMA_CCR_TCIE_Msk | DMA_CCR_DTEIE_Msk);

    LPDMA1_CH0->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos));
    LPDMA1_CH0->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

    LPDMA1_CH0->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    LPDMA1_CH0->CTR2 |= (21U << DMA_CTR2_REQSEL_Pos);

    LPDMA1_CH0->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA1_CH0->CCR |= DMA_CCR_TCIE_Msk | DMA_CCR_DTEIE_Msk;

    NVIC_ClearPendingIRQ(LPDMA1_CH0_IRQn);
    NVIC_SetPriority(LPDMA1_CH0_IRQn, 5U);
    NVIC_EnableIRQ(LPDMA1_CH0_IRQn);

    UART5->CR3 |= (0x1U << USART_CR3_DMAT_Pos);
}

static void uart5_rx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;
    (void)RCC->AHB1ENR;

    LPDMA1_CH1->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    LPDMA1_CH1->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos));
    LPDMA1_CH1->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

    LPDMA1_CH1->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    LPDMA1_CH1->CTR2 |= (20U << DMA_CTR2_REQSEL_Pos);

    LPDMA1_CH1->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    LPDMA1_CH1->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC_ClearPendingIRQ(LPDMA1_CH1_IRQn);
    NVIC_SetPriority(LPDMA1_CH1_IRQn, 5U);
    NVIC_EnableIRQ(LPDMA1_CH1_IRQn);

    UART5->CR3 |= (0x1U << USART_CR3_DMAR_Pos);
}

static void uart5_rx_dma_start(void)
{
    uart5_rx_event = 0U;
    uart5_rx_dma_error = 0U;

    LPDMA1_CH1->CSAR = (uint32_t)&UART5->RDR;
    LPDMA1_CH1->CDAR = (uint32_t)uart5_rx_dma_buffer;
    LPDMA1_CH1->CBR1 = UART5_RX_BUFFER_SIZE;

    LPDMA1_CH1->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA1_CH1->CCR |= DMA_CCR_EN_Msk;
}

void uart5_init(uint32_t baudrate)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    GPIOB->MODER &= ~(0x3U << (UART5_TX * 2U) | 0x3U << (UART5_RX * 2U));
    GPIOB->MODER |= (0x2U << (UART5_TX * 2U) | 0x2U << (UART5_RX * 2U));

    GPIOB->OTYPER &= ~(0x1U << UART5_TX | 0x1U << UART5_RX);
    GPIOB->PUPDR &= ~(0x3U << (UART5_TX * 2U) | 0x3U << (UART5_RX * 2U));
    GPIOB->OSPEEDR &= ~(0x3U << (UART5_TX * 2U) | 0x3U << (UART5_RX * 2U));

    GPIOB->AFR[0] &= ~(0xFU << (UART5_TX * 4U) | 0xFU << (UART5_RX * 4U));
    GPIOB->AFR[0] |= (0xEU << (UART5_TX * 4U) | 0xEU << (UART5_RX * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_UART5EN;
    (void)RCC->APB1LENR;

    UART5->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    UART5->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    UART5->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    uint32_t pclk = apb1_pclk_hz();

    UART5->PRESC = 0x0U;

    UART5->BRR = compute_uart_div(pclk, baudrate);

    UART5->CR1 |= (0x1U << USART_CR1_IDLEIE_Pos);
    UART5->CR1 &= ~(USART_CR1_RXNEIE_Msk | USART_CR1_TXEIE_Msk);
    UART5->CR1 |= (0x1U << USART_CR1_RE_Pos) | (0x1U << USART_CR1_TE_Pos);

    NVIC_ClearPendingIRQ(UART5_IRQn);
    NVIC_SetPriority(UART5_IRQn, 5U);
    NVIC_EnableIRQ(UART5_IRQn);

    UART5->CR1 |= USART_CR1_UE;

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);

    while ((UART5->ISR & ack_mask) != ack_mask) {}

    uart5_tx_dma_init();
    uart5_rx_dma_init();
    uart5_rx_dma_start();
}

uint8_t uart5_config(const serial_config_t *config)
{
    if (config == NULL) return 0U;
    if (config->baudrate == 0U) return 0U;
    if (config->data_bits != 8U) return 0U;
    if (config->parity != 0U) return 0U;
    if ((config->stop_bits != 1U) && (config->stop_bits != 2U)) return 0U;

    if ((LPDMA1_CH1->CCR & DMA_CCR_EN_Msk) != 0U)
    {
        if ((LPDMA1_CH0->CCR & DMA_CCR_EN_Msk) != 0U)
        {
            return 0U;
        }

        LPDMA1_CH1->CCR |= DMA_CCR_SUSP_Msk;

        while ((LPDMA1_CH1->CSR & DMA_CSR_SUSPF_Msk) == 0U){}
        while ((LPDMA1_CH1->CSR & DMA_CSR_IDLEF_Msk) == 0U){}

        LPDMA1_CH1->CCR |= DMA_CCR_RESET_Msk;

        while ((LPDMA1_CH1->CCR & DMA_CCR_EN_Msk) != 0U){}
    }

    UART5->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    UART5->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    UART5->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    switch (config->data_bits)
    {
        case 7U:
            UART5->CR1 |= USART_CR1_M1;
            break;
        case 8U:
            UART5->CR1 &= ~(USART_CR1_M0_Msk | USART_CR1_M1_Msk);
            break;
        case 9U:
            UART5->CR1 |= USART_CR1_M0;
            break;
        default:
            return 0U;
    }

    switch (config->parity)
    {
        case 0U:
            UART5->CR1 &= ~(USART_CR1_PCE_Msk | USART_CR1_PS_Msk);
            break;
        case 1U:
            UART5->CR1 &= ~(USART_CR1_PS_Msk);
            UART5->CR1 |= USART_CR1_PCE_Msk;
            break;
        case 2U:
            UART5->CR1 |= USART_CR1_PCE_Msk | USART_CR1_PS_Msk;
            break;
        default:
            return 0U;
    }

    switch (config->stop_bits)
    {
        case 1U:
            UART5->CR2 &= ~USART_CR2_STOP_Msk;
            break;
        case 2U:
            UART5->CR2 |= (0x2U << USART_CR2_STOP_Pos);
            break;
        default:
            return 0U;
    }

    uint32_t pclk = apb1_pclk_hz();

    UART5->PRESC = 0x0U;
    UART5->BRR = compute_uart_div(pclk, config->baudrate);
    UART5->CR1 |= USART_CR1_RE_Msk | USART_CR1_TE_Msk;
    UART5->CR1 |= USART_CR1_UE;

    NVIC_ClearPendingIRQ(UART5_IRQn);
    NVIC_SetPriority(UART5_IRQn, 5U);
    NVIC_EnableIRQ(UART5_IRQn);

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);
    while ((UART5->ISR & ack_mask) != ack_mask) {}
    
    UART5->ICR = USART_ICR_IDLECF_Msk;

    uart5_rx_dma_start();
    
    return 1U;
}

uint8_t uart5_dma_write(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U) || (length > UART5_TX_BUFFER_SIZE))
    {
        return 0U;
    }

    if ((LPDMA1_CH0->CCR & DMA_CCR_EN_Msk) != 0U)
    {
        return 0U;
    }

    uart5_tx_dma_error = 0U;

    for (uint16_t i = 0; i < length; i++)
    {
        uart5_tx_buffer[i] = data[i];
    }

    LPDMA1_CH0->CSAR = (uint32_t)uart5_tx_buffer;
    LPDMA1_CH0->CDAR = (uint32_t)&UART5->TDR;
    LPDMA1_CH0->CBR1 = length;

    LPDMA1_CH0->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA1_CH0->CCR |= DMA_CCR_EN_Msk;

    return 1U;
}

void uart5_process(void)
{
    if (uart5_tx_event != 0U)
    {
        uart5_tx_event = 0U;
        uart5_tx_callback((uart5_tx_dma_error == 0U) ? 1U : 0U);
    }

    if (uart5_rx_event == 0U)
    {
        return;
    }

    uart5_rx_event = 0U;

    LPDMA1_CH1->CCR |= DMA_CCR_SUSP_Msk;

    while ((LPDMA1_CH1->CSR & DMA_CSR_SUSPF_Msk) == 0U){}
    while ((LPDMA1_CH1->CSR & DMA_CSR_IDLEF_Msk) == 0U){}

    uint16_t remaining = (uint16_t)((LPDMA1_CH1->CBR1 & DMA_CBR1_BNDT_Msk) >> DMA_CBR1_BNDT_Pos);
    uint16_t received = (uint16_t)(UART5_RX_BUFFER_SIZE - remaining);

    LPDMA1_CH1->CCR |= DMA_CCR_RESET_Msk;

    while ((LPDMA1_CH1->CCR & DMA_CCR_EN_Msk) != 0U){}

    if ((received > 0U) && (uart5_rx_dma_error == 0U))
    {
        uart5_rx_callback((const uint8_t *)uart5_rx_dma_buffer, received);
    }
    uart5_rx_dma_start();
}

void UART5_IRQHandler(void)
{
    uint32_t status = UART5->ISR;

    if ((status & USART_ISR_IDLE_Msk) != 0U)
    {
        UART5->ICR = USART_ICR_IDLECF_Msk;
        uart5_rx_event = 1U;
    }

    if ((status & USART_ISR_TC_Msk) != 0U)
    {
        UART5->ICR = USART_ICR_TCCF_Msk;
        UART5->CR1 &= ~USART_CR1_TCIE_Msk;

        uart5_tx_event = 1U;
    }
}

void LPDMA1_CH0_IRQHandler(void)
{
    uint32_t status = LPDMA1_CH0->CSR;

    if ((status & DMA_CSR_DTEF_Msk) != 0U)
    {
        LPDMA1_CH0->CFCR = DMA_CFCR_DTEF_Msk;
        uart5_tx_dma_error = 1U;
    }

    if ((status & DMA_CSR_TCF_Msk) != 0U)
    {
        LPDMA1_CH0->CFCR = DMA_CFCR_TCF_Msk;
        UART5->CR1 |= USART_CR1_TCIE_Msk;
    }
}

void LPDMA1_CH1_IRQHandler(void)
{
    uint32_t status = LPDMA1_CH1->CSR;

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        LPDMA1_CH1->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        uart5_rx_dma_error = 1U;
    }
}