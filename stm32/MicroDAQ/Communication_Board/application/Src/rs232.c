#include "rs232.h"
#include <stddef.h>
#include <string.h>

static uint8_t rs232_tx_buffer[RS232_TX_BUFFER_SIZE];
static volatile uint8_t rs232_rx_dma_buffer[RS232_RX_BUFFER_SIZE];

static volatile uint8_t rs232_rx_event = 0U;
static volatile uint8_t rs232_tx_event = 0U;

static volatile uint8_t rs232_rx_dma_error = 0U;
static volatile uint8_t rs232_tx_dma_error = 0U;

static uint32_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

static void rs232_tx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;
    (void)RCC->AHB1ENR;

    LPDMA1_CH2->CCR &= ~(DMA_CCR_EN_Msk | DMA_CCR_TCIE_Msk | DMA_CCR_DTEIE_Msk);

    LPDMA1_CH2->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos));
    LPDMA1_CH2->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

    LPDMA1_CH2->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    LPDMA1_CH2->CTR2 |= (19U << DMA_CTR2_REQSEL_Pos);

    LPDMA1_CH2->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA1_CH2->CCR |= DMA_CCR_TCIE_Msk | DMA_CCR_DTEIE_Msk;

    NVIC_ClearPendingIRQ(LPDMA1_CH2_IRQn);
    NVIC_SetPriority(LPDMA1_CH2_IRQn, 5U);
    NVIC_EnableIRQ(LPDMA1_CH2_IRQn);

    UART4->CR3 |= (0x1U << USART_CR3_DMAT_Pos);
}

static void rs232_rx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA1EN;
    (void)RCC->AHB1ENR;

    LPDMA1_CH3->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    LPDMA1_CH3->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos));
    LPDMA1_CH3->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

    LPDMA1_CH3->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    LPDMA1_CH3->CTR2 |= (18U << DMA_CTR2_REQSEL_Pos);

    LPDMA1_CH3->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    LPDMA1_CH3->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC_ClearPendingIRQ(LPDMA1_CH3_IRQn);
    NVIC_SetPriority(LPDMA1_CH3_IRQn, 5U);
    NVIC_EnableIRQ(LPDMA1_CH3_IRQn);

    UART4->CR3 |= (0x1U << USART_CR3_DMAR_Pos);
}

static void rs232_rx_dma_start(void)
{
    rs232_rx_event = 0U;
    rs232_rx_dma_error = 0U;

    LPDMA1_CH3->CSAR = (uint32_t)&UART4->RDR;
    LPDMA1_CH3->CDAR = (uint32_t)rs232_rx_dma_buffer;
    LPDMA1_CH3->CBR1 = RS232_RX_BUFFER_SIZE;

    LPDMA1_CH3->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA1_CH3->CCR |= DMA_CCR_EN_Msk;
}

void rs232_init(uint32_t baudrate)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER &= ~(0x3U << (UART4_TX * 2U) | 0x3U << (UART4_RX * 2U));
    GPIOA->MODER |= (0x2U << (UART4_TX * 2U) | 0x2U << (UART4_RX * 2U));

    GPIOA->OTYPER &= ~(0x1U << UART4_TX | 0x1U << UART4_RX);
    GPIOA->PUPDR &= ~(0x3U << (UART4_TX * 2U) | 0x3U << (UART4_RX * 2U));
    GPIOA->OSPEEDR &= ~(0x3U << (UART4_TX * 2U) | 0x3U << (UART4_RX * 2U));

    GPIOA->AFR[0] &= ~(0xFU << (UART4_TX * 4U) | 0xFU << (UART4_RX * 4U));
    GPIOA->AFR[0] |= (0x8U << (UART4_TX * 4U) | 0x8U << (UART4_RX * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_UART4EN;
    (void)RCC->APB1LENR;

    UART4->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    UART4->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    UART4->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    uint32_t pclk = apb1_pclk_hz();

    UART4->PRESC = 0x0U;

    UART4->BRR = compute_uart_div(pclk, baudrate);

    UART4->CR1 |= (0x1U << USART_CR1_IDLEIE_Pos);
    UART4->CR1 &= ~(USART_CR1_RXNEIE_Msk | USART_CR1_TXEIE_Msk);
    UART4->CR1 |= (0x1U << USART_CR1_RE_Pos) | (0x1U << USART_CR1_TE_Pos);

    NVIC_ClearPendingIRQ(UART4_IRQn);
    NVIC_SetPriority(UART4_IRQn, 5U);
    NVIC_EnableIRQ(UART4_IRQn);

    UART4->CR1 |= USART_CR1_UE;

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);

    while ((UART4->ISR & ack_mask) != ack_mask) {}

    rs232_tx_dma_init();
    rs232_rx_dma_init();
    rs232_rx_dma_start();
}

uint8_t rs232_config(const serial_config_t *config)
{
    if (config == NULL) return 0U;
    if (config->baudrate == 0U) return 0U;
    if (config->data_bits != 8U) return 0U;
    if (config->parity != 0U) return 0U;
    if ((config->stop_bits != 1U) && (config->stop_bits != 2U)) return 0U;

    if ((LPDMA1_CH3->CCR & DMA_CCR_EN_Msk) != 0U)
    {
        if ((LPDMA1_CH2->CCR & DMA_CCR_EN_Msk) != 0U)
        {
            return 0U;
        }

        LPDMA1_CH3->CCR |= DMA_CCR_SUSP_Msk;

        while ((LPDMA1_CH3->CSR & DMA_CSR_SUSPF_Msk) == 0U){}
        while ((LPDMA1_CH3->CSR & DMA_CSR_IDLEF_Msk) == 0U){}

        LPDMA1_CH3->CCR |= DMA_CCR_RESET_Msk;

        while ((LPDMA1_CH3->CCR & DMA_CCR_EN_Msk) != 0U){}
    }

    UART4->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    UART4->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    UART4->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    switch (config->data_bits)
    {
        case 7U:
            UART4->CR1 |= USART_CR1_M1;
            break;
        case 8U:
            UART4->CR1 &= ~(USART_CR1_M0_Msk | USART_CR1_M1_Msk);
            break;
        case 9U:
            UART4->CR1 |= USART_CR1_M0;
            break;
        default:
            return 0U;
    }

    switch (config->parity)
    {
        case 0U:
            UART4->CR1 &= ~(USART_CR1_PCE_Msk | USART_CR1_PS_Msk);
            break;
        case 1U:
            UART4->CR1 &= ~(USART_CR1_PS_Msk);
            UART4->CR1 |= USART_CR1_PCE_Msk;
            break;
        case 2U:
            UART4->CR1 |= USART_CR1_PCE_Msk | USART_CR1_PS_Msk;
            break;
        default:
            return 0U;
    }

    switch (config->stop_bits)
    {
        case 1U:
            UART4->CR2 &= ~USART_CR2_STOP_Msk;
            break;
        case 2U:
            UART4->CR2 |= (0x2U << USART_CR2_STOP_Pos);
            break;
        default:
            return 0U;
    }

    uint32_t pclk = apb1_pclk_hz();

    UART4->PRESC = 0x0U;
    UART4->BRR = compute_uart_div(pclk, config->baudrate);
    UART4->CR1 |= USART_CR1_RE_Msk | USART_CR1_TE_Msk;
    UART4->CR1 |= USART_CR1_UE;

    NVIC_ClearPendingIRQ(UART4_IRQn);
    NVIC_SetPriority(UART4_IRQn, 5U);
    NVIC_EnableIRQ(UART4_IRQn);

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);
    while ((UART4->ISR & ack_mask) != ack_mask) {}
    
    UART4->ICR = USART_ICR_IDLECF_Msk;

    rs232_rx_dma_start();
    
    return 1U;
}

uint8_t rs232_dma_write(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U) || (length > RS232_TX_BUFFER_SIZE))
    {
        return 0U;
    }

    if ((LPDMA1_CH2->CCR & DMA_CCR_EN_Msk) != 0U)
    {
        return 0U;
    }

    rs232_tx_dma_error = 0U;

    for (uint16_t i = 0; i < length; i++)
    {
        rs232_tx_buffer[i] = data[i];
    }

    LPDMA1_CH2->CSAR = (uint32_t)rs232_tx_buffer;
    LPDMA1_CH2->CDAR = (uint32_t)&UART4->TDR;
    LPDMA1_CH2->CBR1 = length;

    LPDMA1_CH2->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA1_CH2->CCR |= DMA_CCR_EN_Msk;

    return 1U;
}

void rs232_process(void)
{
    if (rs232_tx_event != 0U)
    {
        rs232_tx_event = 0U;
        rs232_tx_callback((rs232_tx_dma_error == 0U) ? 1U : 0U);
    }

    if (rs232_rx_event == 0U)
    {
        return;
    }

    rs232_rx_event = 0U;

    LPDMA1_CH3->CCR |= DMA_CCR_SUSP_Msk;

    while ((LPDMA1_CH3->CSR & DMA_CSR_SUSPF_Msk) == 0U){}
    while ((LPDMA1_CH3->CSR & DMA_CSR_IDLEF_Msk) == 0U){}

    uint16_t remaining = (uint16_t)((LPDMA1_CH3->CBR1 & DMA_CBR1_BNDT_Msk) >> DMA_CBR1_BNDT_Pos);
    uint16_t received = (uint16_t)(RS232_RX_BUFFER_SIZE - remaining);

    LPDMA1_CH3->CCR |= DMA_CCR_RESET_Msk;

    while ((LPDMA1_CH3->CCR & DMA_CCR_EN_Msk) != 0U){}

    if ((received > 0U) && (rs232_rx_dma_error == 0U))
    {
        rs232_rx_callback((const uint8_t *)rs232_rx_dma_buffer, received);
    }
    rs232_rx_dma_start();
}

void UART4_IRQHandler(void)
{
    uint32_t status = UART4->ISR;

    if ((status & USART_ISR_IDLE_Msk) != 0U)
    {
        UART4->ICR = USART_ICR_IDLECF_Msk;
        rs232_rx_event = 1U;
    }

    if ((status & USART_ISR_TC_Msk) != 0U)
    {
        UART4->ICR = USART_ICR_TCCF_Msk;
        UART4->CR1 &= ~USART_CR1_TCIE_Msk;

        rs232_tx_event = 1U;
    }
}

void LPDMA1_CH2_IRQHandler(void)
{
    uint32_t status = LPDMA1_CH2->CSR;

    if ((status & DMA_CSR_DTEF_Msk) != 0U)
    {
        LPDMA1_CH2->CFCR = DMA_CFCR_DTEF_Msk;
        rs232_tx_dma_error = 1U;
    }

    if ((status & DMA_CSR_TCF_Msk) != 0U)
    {
        LPDMA1_CH2->CFCR = DMA_CFCR_TCF_Msk;
        UART4->CR1 |= USART_CR1_TCIE_Msk;
    }
}

void LPDMA1_CH3_IRQHandler(void)
{
    uint32_t status = LPDMA1_CH3->CSR;

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        LPDMA1_CH3->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        rs232_rx_dma_error = 1U;
    }
}