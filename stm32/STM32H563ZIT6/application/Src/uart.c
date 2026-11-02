#include "uart.h"
#include "stm32h563xx.h"
#include "support.h"

static volatile uint8_t uart2_rx_dma_buffer[UART2_RX_BUFFER_SIZE];
static uint32_t uart2_rx_dma_node_dar __attribute__((aligned(4)));
static volatile uint8_t uart2_rx_event = 0U;
static volatile uint8_t uart2_rx_dma_error = 0U;
static volatile uint8_t uart2_tx_dma_error = 0U;
static uint16_t uart2_rx_old_pos = 0U;

static uint32_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

static void uart2_tx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel0->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    GPDMA1_Channel0->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel0->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

    GPDMA1_Channel0->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel0->CTR2 |= (0x18U << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_DREQ_Pos);

    GPDMA1_Channel0->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel0->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC->IPR[27] = 0x50U;
    NVIC->ICPR[0] = (1U << 27U);
    NVIC->ISER[0] = (1U << 27U);

    USART2->CR3 |= (0x1U << USART_CR3_DMAT_Pos);
}

static void uart2_rx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel1->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    GPDMA1_Channel1->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel1->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

    GPDMA1_Channel1->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_DREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel1->CTR2 |= (0x17U << DMA_CTR2_REQSEL_Pos);

    uart2_rx_dma_node_dar = (uint32_t)uart2_rx_dma_buffer;
    uint32_t node_addr = (uint32_t)&uart2_rx_dma_node_dar;

    GPDMA1_Channel1->CLBAR = node_addr & 0xFFFF0000UL;
    GPDMA1_Channel1->CLLR = (node_addr & 0x0000FFFCUL) | (0x1UL << DMA_CLLR_UDA_Pos);

    GPDMA1_Channel1->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel1->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC->IPR[28] = 0x50U;
    NVIC->ICPR[0] = (1U << 28U);
    NVIC->ISER[0] = (1U << 28U);

    USART2->CR3 |= (0x1U << USART_CR3_DMAR_Pos);
}

static void uart2_rx_dma_start(void)
{
    uart2_rx_event = 0U;
    uart2_rx_dma_error = 0U;
    uart2_rx_old_pos = 0U;

    GPDMA1_Channel1->CSAR = (uint32_t)&USART2->RDR;
    GPDMA1_Channel1->CDAR = (uint32_t)uart2_rx_dma_buffer;
    GPDMA1_Channel1->CBR1 = UART2_RX_BUFFER_SIZE;

    GPDMA1_Channel1->CFCR = (1U << DMA_CFCR_TCF_Pos) | (1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel1->CCR |= (1U << DMA_CCR_EN_Pos);
}

void uart2_init(uint32_t baudrate)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    (void)RCC->AHB2ENR;

    GPIOD->MODER &= ~(0x3U << (UART2_TX * 2U) | 0x3U << (UART2_RX * 2U));
    GPIOD->MODER |= (0x2U << (UART2_TX * 2U) | 0x2U << (UART2_RX * 2U));

    GPIOD->OTYPER &= ~(0x1U << UART2_TX | 0x1U << UART2_RX);
    GPIOD->PUPDR &= ~(0x3U << (UART2_TX * 2U) | 0x3U << (UART2_RX * 2U));
    GPIOD->OSPEEDR &= ~(0x3U << (UART2_TX * 2U) | 0x3U << (UART2_RX * 2U));

    GPIOD->AFR[0] &= ~(0xFU << (UART2_TX * 4U) | 0xFU << (UART2_RX * 4U));
    GPIOD->AFR[0] |= (0x7U << (UART2_TX * 4U) | 0x7U << (UART2_RX * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_USART2EN;
    (void)RCC->APB1LENR;

    USART2->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    USART2->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    USART2->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    uint32_t pclk = apb1_pclk_hz();

    USART2->PRESC = 0x0U;

    USART2->BRR = compute_uart_div(pclk, baudrate);

    USART2->CR1 |= (0x1U << USART_CR1_IDLEIE_Pos);

    USART2->CR1 &= ~(USART_CR1_RXNEIE_Msk | USART_CR1_TXEIE_Msk);

    USART2->CR1 |= (0x1U << USART_CR1_RE_Pos) | (0x1U << USART_CR1_TE_Pos);

    NVIC->IPR[59] = 0x50U;
    NVIC->ICPR[1] = (1UL << 27U);
    NVIC->ISER[1] = (1UL << 27U);

    USART2->CR1 |= USART_CR1_UE;

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);

    while ((USART2->ISR & ack_mask) != ack_mask) {}

    uart2_tx_dma_init();
    uart2_rx_dma_init();
    uart2_rx_dma_start();
}

uint8_t uart2_dma_write(const uint8_t *data, uint16_t length)
{
    if (length == 0U)
    {
        return 0U;
    }

    if ((GPDMA1_Channel0->CCR & (1U << DMA_CCR_EN_Pos)) != 0U)
    {
        return 0U;
    }

    uart2_tx_dma_error = 0U;

    GPDMA1_Channel0->CSAR = (uint32_t)data;
    GPDMA1_Channel0->CDAR = (uint32_t)&USART2->TDR;
    GPDMA1_Channel0->CBR1 = length;

    GPDMA1_Channel0->CFCR = (1U << DMA_CFCR_TCF_Pos) | (1U << DMA_CFCR_DTEF_Pos);

    GPDMA1_Channel0->CCR |= (1U << DMA_CCR_EN_Pos);

    return 1U;
}

void uart2_process(void)
{
    if (!uart2_rx_event)
    {
        return;
    }

    uart2_rx_event = 0U;

    uint16_t remaining = (uint16_t)((GPDMA1_Channel1->CBR1 & DMA_CBR1_BNDT_Msk) >> DMA_CBR1_BNDT_Pos);

    uint16_t current_pos = UART2_RX_BUFFER_SIZE - remaining;

    if (current_pos > uart2_rx_old_pos)
    {
        uart2_rx_callback(
            (const uint8_t *)&uart2_rx_dma_buffer[uart2_rx_old_pos],
            current_pos - uart2_rx_old_pos
        );
    }
    else if (current_pos < uart2_rx_old_pos)
    {
        uart2_rx_callback(
            (const uint8_t *)&uart2_rx_dma_buffer[uart2_rx_old_pos],
            UART2_RX_BUFFER_SIZE - uart2_rx_old_pos
        );

        if (current_pos > 0U)
        {
            uart2_rx_callback(
                (const uint8_t *)&uart2_rx_dma_buffer[0],
                current_pos
            );
        }
    }

    uart2_rx_old_pos = current_pos;
}

void USART2_IRQHandler(void)
{
    if ((USART2->ISR & (0x1U << USART_ISR_IDLE_Pos)) != 0U)
    {
        USART2->ICR = (0x1U << USART_ICR_IDLECF_Pos);
        uart2_rx_event = 1U;
    }
}

void GPDMA1_CH0_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel0->CSR;

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        GPDMA1_Channel0->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        uart2_tx_dma_error = 1U;
    }
}

void GPDMA1_CH1_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel1->CSR;

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        GPDMA1_Channel1->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        uart2_rx_dma_error = 1U;
    }
}