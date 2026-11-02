#include "uart.h"
#include "stm32h563xx.h"
#include "support.h"

volatile uint8_t uart3_rx_buffer[UART3_RX_BUFFER_SIZE];
volatile uint16_t uart3_rx_head = 0U;
volatile uint16_t uart3_rx_tail = 0U;
volatile uint32_t uart3_rx_overflow_count = 0U;

volatile uint8_t uart3_tx_buffer[UART3_TX_BUFFER_SIZE];
volatile uint16_t uart3_tx_head = 0U;
volatile uint16_t uart3_tx_tail = 0U;

volatile uint8_t uart3_dma_tx_done = 0U;
volatile uint8_t uart3_dma_tx_error = 0U;

volatile uint8_t uart3_dma_rx_done = 0U;
volatile uint8_t uart3_dma_rx_error = 0U;

volatile uint8_t uart3_idle_detected = 0U;

volatile uint8_t uart3_dma_rx_byte = 0;
volatile uint8_t uart3_dma_rx_buffer[UART3_RX_BUFFER_SIZE];

uint16_t uart3_rx_old_pos = 0U;
uint16_t uart3_rx_current_pos = 0U;


static uint32_t uart3_rx_dma_node_dar __attribute__((aligned(4)));

static uint32_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

void uart3_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    (void)RCC->AHB2ENR;

    GPIOD->MODER &= ~(0x3U << (UART3_TX * 2U) | 0x3U << (UART3_RX * 2U));
    GPIOD->MODER |= (0x2U << (UART3_TX * 2U) | 0x2U << (UART3_RX * 2U));

    GPIOD->OTYPER &= ~(0x1U << UART3_TX | 0x1U << UART3_RX);
    GPIOD->PUPDR &= ~(0x3U << (UART3_TX * 2U) | 0x3U << (UART3_RX * 2U));
    GPIOD->OSPEEDR &= ~(0x3U << (UART3_TX * 2U) | 0x3U << (UART3_RX * 2U));

    GPIOD->AFR[1] &= ~(0xFU << ((UART3_TX - 8U) * 4U) | 0xFU << ((UART3_RX - 8U) * 4U));
    GPIOD->AFR[1] |= (0x7U << ((UART3_TX - 8U) * 4U) | 0x7U << ((UART3_RX - 8U) * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_USART3EN;
    (void)RCC->APB1LENR;

    USART3->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    USART3->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    USART3->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    uint32_t pclk = apb1_pclk_hz();

    USART3->PRESC = 0x0U;

    USART3->BRR = compute_uart_div(pclk, 115200U);

    // USART3->CR1 |= (0x1U << USART_CR1_RXNEIE_Pos) | (0x1U << USART_CR1_TXEIE_Pos);
    USART3->CR1 |= (0x1U << USART_CR1_IDLEIE_Pos);

    USART3->CR1 &= ~(USART_CR1_RXNEIE_Msk | USART_CR1_TXEIE_Msk);

    USART3->CR1 |= (0x1U << USART_CR1_RE_Pos) | (0x1U << USART_CR1_TE_Pos);

    NVIC->IPR[60] = 0x50U;
    NVIC->ICPR[1] = (1UL << 28U);
    NVIC->ISER[1] = (1UL << 28U);

    USART3->CR1 |= USART_CR1_UE;

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);

    while ((USART3->ISR & ack_mask) != ack_mask) {}
}

void uart3_tx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel0->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    GPDMA1_Channel0->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel0->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

    GPDMA1_Channel0->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel0->CTR2 |= (0x1AU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_DREQ_Pos);

    GPDMA1_Channel0->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel0->CCR |= (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC->IPR[27] = 0x50U;
    NVIC->ICPR[0] = (1U << 27U);
    NVIC->ISER[0] = (1U << 27U);

    USART3->CR3 |= (0x1U << USART_CR3_DMAT_Pos);
}

void uart3_rx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel1->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    GPDMA1_Channel1->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel1->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

    GPDMA1_Channel1->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_DREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel1->CTR2 |= (0x19U << DMA_CTR2_REQSEL_Pos);

    uart3_rx_dma_node_dar = (uint32_t)uart3_dma_rx_buffer;
    uint32_t node_addr = (uint32_t)&uart3_rx_dma_node_dar;

    GPDMA1_Channel1->CLBAR = node_addr & 0xFFFF0000UL;
    GPDMA1_Channel1->CLLR = (node_addr & 0x0000FFFCUL) | (0x1UL << DMA_CLLR_UDA_Pos);

    GPDMA1_Channel1->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel1->CCR |= (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC->IPR[28] = 0x50U;
    NVIC->ICPR[0] = (1U << 28U);
    NVIC->ISER[0] = (1U << 28U);

    USART3->CR3 |= (0x1U << USART_CR3_DMAR_Pos);
}

uint8_t uart3_dma_write(const uint8_t *data, uint16_t length)
{
    if ((GPDMA1_Channel0->CCR & (1U << DMA_CCR_EN_Pos)) != 0U)
    {
        return 0U;
    }

    GPDMA1_Channel0->CSAR = (uint32_t)data;
    GPDMA1_Channel0->CDAR = (uint32_t)&USART3->TDR;
    GPDMA1_Channel0->CBR1 = length;

    GPDMA1_Channel0->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);

    uart3_dma_tx_done = 0U;
    uart3_dma_tx_error = 0U;
    
    GPDMA1_Channel0->CCR |= (0x1U << DMA_CCR_EN_Pos);

    return 1U;
}

uint8_t uart3_dma_read(volatile uint8_t *buffer, uint16_t length)
{
    if (length == 0U)
    {
        return 0U;
    }

    if ((GPDMA1_Channel1->CCR & (1U << DMA_CCR_EN_Pos)) != 0U)
    {
        return 0U;
    }

    GPDMA1_Channel1->CSAR = (uint32_t)&USART3->RDR;
    GPDMA1_Channel1->CDAR = (uint32_t)buffer;
    GPDMA1_Channel1->CBR1 = length;

    GPDMA1_Channel1->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);

    uart3_dma_rx_done = 0U;
    uart3_dma_rx_error = 0U;

    GPDMA1_Channel1->CCR |= (0x1U << DMA_CCR_EN_Pos);

    return 1U;
}

void uart3_write_byte(uint8_t data)
{
    while ((USART3->ISR & (0x1U << USART_ISR_TXE_Pos)) == 0U) {}
    USART3->TDR = data;
}

uint8_t uart3_write_byte_irq(uint8_t data)
{
    uint16_t next_head = (uart3_tx_head + 1U) & (UART3_TX_BUFFER_SIZE - 1U);

    if (next_head == uart3_tx_tail) {
        return 0U;
    }

    uart3_tx_buffer[uart3_tx_head] = data;
    uart3_tx_head = next_head;

    USART3->CR1 |= (0x1U << USART_CR1_TXEIE_Pos);
    return 1U;
}

void uart3_write_string_irq(const char *str)
{
    while (*str != '\0')
    {
        while (!uart3_write_byte_irq((uint8_t)(*str))){}
        str++;
    }
}

void uart3_write_string(const char *str)
{
    while (*str != '\0')
    {
        uart3_write_byte((uint8_t)(*str));
        str++;
    }
}

// uint8_t uart3_read_byte(void)
// {
//     while ((USART3->ISR & (0x1U << 5U)) == 0U) {}
//     return (uint8_t)(USART3->RDR & 0xFFU);
// }

uint8_t uart3_read_byte_nonblocking(uint8_t *data){
    if ((USART3->ISR & (0x1U << USART_ISR_RXNE_Pos)) != 0U) {
        *data = (uint8_t)(USART3->RDR & 0xFFU);
        return 1U;
    } else {
        return 0U;
    }
}

uint8_t uart3_read_byte(uint8_t *data)
{
    if (uart3_rx_head == uart3_rx_tail)
    {
        return 0U;
    }

    *data = uart3_rx_buffer[uart3_rx_tail];

    uart3_rx_tail =
        (uart3_rx_tail + 1U) % UART3_RX_BUFFER_SIZE;

    return 1U;
}

void USART3_IRQHandler(void)
{
    if ((USART3->ISR & (0x1U << USART_ISR_RXNE_Pos)) != 0U) {
        uint8_t uart3_rx_data = (uint8_t)(USART3->RDR & 0xFFU);

        uint16_t next_head = (uart3_rx_head + 1U) & (UART3_RX_BUFFER_SIZE - 1U);
        if (next_head != uart3_rx_tail) {
            uart3_rx_buffer[uart3_rx_head] = uart3_rx_data;
            uart3_rx_head = next_head;
        } else {
            uart3_rx_overflow_count++;
        }
    }

    if (((USART3->ISR & (0x1U << USART_ISR_TXE_Pos)) != 0U) &&
    ((USART3->CR1 & (0x1U << USART_CR1_TXEIE_Pos)) != 0U)) 
    {
        if (uart3_tx_head != uart3_tx_tail) {
            USART3->TDR = uart3_tx_buffer[uart3_tx_tail];
            uart3_tx_tail = (uart3_tx_tail + 1U) & (UART3_TX_BUFFER_SIZE - 1U);
        } else {
            USART3->CR1 &= ~(0x1U << USART_CR1_TXEIE_Pos);
        }
    }

    if ((USART3->ISR & (0x1U << USART_ISR_IDLE_Pos)) != 0U)
    {
        USART3->ICR = (0x1U << USART_ICR_IDLECF_Pos);
        uint32_t bndt = (GPDMA1_Channel1->CBR1 & DMA_CBR1_BNDT_Msk) >> DMA_CBR1_BNDT_Pos;
        uart3_rx_current_pos = UART3_RX_BUFFER_SIZE - bndt;
        uart3_idle_detected = 1U;
    }
}

void GPDMA1_CH0_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel0->CSR;

    if ((status & (1U << DMA_CSR_TCF_Pos)) != 0U)
    {
        GPDMA1_Channel0->CFCR = (1U << DMA_CFCR_TCF_Pos);
        uart3_dma_tx_done = 1U;
    }

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        GPDMA1_Channel0->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        uart3_dma_tx_error = 1U;
    }
}

void GPDMA1_CH1_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel1->CSR;

    if ((status & (1U << DMA_CSR_TCF_Pos)) != 0U)
    {
        GPDMA1_Channel1->CFCR = (1U << DMA_CFCR_TCF_Pos);
        uart3_dma_rx_done = 1U;
    }

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        GPDMA1_Channel1->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        uart3_dma_rx_error = 1U;
    }
}

void uart3_rx_process(const uint8_t *data, uint16_t length)
{
}