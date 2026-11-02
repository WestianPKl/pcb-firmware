#include "rs485.h"
#include "output.h"
#include <stddef.h>
#include <string.h>

static uint8_t rs485_tx_buffer[RS485_TX_BUFFER_SIZE];
static volatile uint8_t rs485_rx_dma_buffer[RS485_RX_BUFFER_SIZE];

static volatile uint8_t rs485_rx_event = 0U;
static volatile uint8_t rs485_tx_event = 0U;

static volatile uint8_t rs485_rx_dma_error = 0U;
static volatile uint8_t rs485_tx_dma_error = 0U;

static uint32_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate;
}

static inline void rs485_de_high(void)
{
    output_on(GPIOB, RS485_DE);
}

static inline void rs485_de_low(void)
{
    output_off(GPIOB, RS485_DE);
}

static void rs485_tx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA2EN;
    (void)RCC->AHB1ENR;

    LPDMA2_CH0->CCR &= ~(DMA_CCR_EN_Msk | DMA_CCR_TCIE_Msk | DMA_CCR_DTEIE_Msk);

    LPDMA2_CH0->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos));
    LPDMA2_CH0->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

    LPDMA2_CH0->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    LPDMA2_CH0->CTR2 |= (15U << DMA_CTR2_REQSEL_Pos);

    LPDMA2_CH0->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA2_CH0->CCR |= DMA_CCR_TCIE_Msk | DMA_CCR_DTEIE_Msk;

    NVIC_ClearPendingIRQ(LPDMA2_CH0_IRQn);
    NVIC_SetPriority(LPDMA2_CH0_IRQn, 5U);
    NVIC_EnableIRQ(LPDMA2_CH0_IRQn);

    USART2->CR3 |= (0x1U << USART_CR3_DMAT_Pos);
}

static void rs485_rx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_LPDMA2EN;
    (void)RCC->AHB1ENR;

    LPDMA2_CH1->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    LPDMA2_CH1->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos));
    LPDMA2_CH1->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

    LPDMA2_CH1->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    LPDMA2_CH1->CTR2 |= (14U << DMA_CTR2_REQSEL_Pos);

    LPDMA2_CH1->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    LPDMA2_CH1->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC_ClearPendingIRQ(LPDMA2_CH1_IRQn);
    NVIC_SetPriority(LPDMA2_CH1_IRQn, 5U);
    NVIC_EnableIRQ(LPDMA2_CH1_IRQn);

    USART2->CR3 |= (0x1U << USART_CR3_DMAR_Pos);
}

static void rs485_rx_dma_start(void)
{
    rs485_rx_event = 0U;
    rs485_rx_dma_error = 0U;

    LPDMA2_CH1->CSAR = (uint32_t)&USART2->RDR;
    LPDMA2_CH1->CDAR = (uint32_t)rs485_rx_dma_buffer;
    LPDMA2_CH1->CBR1 = RS485_RX_BUFFER_SIZE;

    LPDMA2_CH1->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA2_CH1->CCR |= DMA_CCR_EN_Msk;
}

void rs485_init(uint32_t baudrate)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    GPIOB->MODER &= ~(0x3U << (UART2_TX * 2U) | 0x3U << (UART2_RX * 2U));
    GPIOB->MODER |= (0x2U << (UART2_TX * 2U) | 0x2U << (UART2_RX * 2U));

    GPIOB->OTYPER &= ~(0x1U << UART2_TX | 0x1U << UART2_RX | 0x1U << RS485_DE);
    GPIOB->PUPDR &= ~(0x3U << (UART2_TX * 2U) | 0x3U << (UART2_RX * 2U));
    GPIOB->OSPEEDR &= ~(0x3U << (UART2_TX * 2U) | 0x3U << (UART2_RX * 2U));

    gpio_output_init(GPIOB, RS485_DE);
    gpio_output_init(GPIOC, RS485_TERM_PIN);

    rs485_de_low();
    GPIOB->AFR[0] &= ~(0xFU << (UART2_TX * 4U) | 0xFU << (UART2_RX * 4U));
    GPIOB->AFR[0] |= (0x7U << (UART2_TX * 4U) | 0x7U << (UART2_RX * 4U));

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

    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_SetPriority(USART2_IRQn, 5U);
    NVIC_EnableIRQ(USART2_IRQn);

    USART2->CR1 |= USART_CR1_UE;

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);

    while ((USART2->ISR & ack_mask) != ack_mask) {}

    rs485_tx_dma_init();
    rs485_rx_dma_init();
    rs485_rx_dma_start();
}

void rs485_termination_on(void)
{
    output_on(GPIOC, RS485_TERM_PIN);
}

void rs485_termination_off(void)
{
    output_off(GPIOC, RS485_TERM_PIN);
}

uint8_t rs485_config(const serial_config_t *config)
{
    if (config == NULL) return 0U;
    if (config->baudrate == 0U) return 0U;
    if (config->data_bits != 8U) return 0U;
    if (config->parity != 0U) return 0U;
    if ((config->stop_bits != 1U) && (config->stop_bits != 2U)) return 0U;

    if ((LPDMA2_CH1->CCR & DMA_CCR_EN_Msk) != 0U)
    {
        if ((LPDMA2_CH0->CCR & DMA_CCR_EN_Msk) != 0U)
        {
            return 0U;
        }

        LPDMA2_CH1->CCR |= DMA_CCR_SUSP_Msk;

        while ((LPDMA2_CH1->CSR & DMA_CSR_SUSPF_Msk) == 0U){}
        while ((LPDMA2_CH1->CSR & DMA_CSR_IDLEF_Msk) == 0U){}

        LPDMA2_CH1->CCR |= DMA_CCR_RESET_Msk;

        while ((LPDMA2_CH1->CCR & DMA_CCR_EN_Msk) != 0U){}
    }

    USART2->CR1 &= ~(USART_CR1_UE_Msk | USART_CR1_RE_Msk | USART_CR1_TE_Msk);
    USART2->CR1 &= ~(USART_CR1_M_Msk | USART_CR1_PS_Msk | USART_CR1_PCE_Msk | USART_CR1_OVER8_Msk | USART_CR1_FIFOEN_Msk);
    USART2->CR2 &= ~(USART_CR2_STOP_Msk | USART_CR2_CLKEN_Msk | USART_CR2_LINEN_Msk);

    switch (config->data_bits)
    {
        case 7U:
            USART2->CR1 |= USART_CR1_M1;
            break;
        case 8U:
            USART2->CR1 &= ~(USART_CR1_M0_Msk | USART_CR1_M1_Msk);
            break;
        case 9U:
            USART2->CR1 |= USART_CR1_M0;
            break;
        default:
            return 0U;
    }

    switch (config->parity)
    {
        case 0U:
            USART2->CR1 &= ~(USART_CR1_PCE_Msk | USART_CR1_PS_Msk);
            break;
        case 1U:
            USART2->CR1 &= ~(USART_CR1_PS_Msk);
            USART2->CR1 |= USART_CR1_PCE_Msk;
            break;
        case 2U:
            USART2->CR1 |= USART_CR1_PCE_Msk | USART_CR1_PS_Msk;
            break;
        default:
            return 0U;
    }

    switch (config->stop_bits)
    {
        case 1U:
            USART2->CR2 &= ~USART_CR2_STOP_Msk;
            break;
        case 2U:
            USART2->CR2 |= (0x2U << USART_CR2_STOP_Pos);
            break;
        default:
            return 0U;
    }

    if (config->rs485_termination == 1U)
    {
       output_on(GPIOC, RS485_TERM_PIN);
    }
    else
    {
        output_off(GPIOC, RS485_TERM_PIN);
    }

    uint32_t pclk = apb1_pclk_hz();

    USART2->PRESC = 0x0U;
    USART2->BRR = compute_uart_div(pclk, config->baudrate);
    USART2->CR1 |= USART_CR1_RE_Msk | USART_CR1_TE_Msk;
    USART2->CR1 |= USART_CR1_UE;

    NVIC_ClearPendingIRQ(USART2_IRQn);
    NVIC_SetPriority(USART2_IRQn, 5U);
    NVIC_EnableIRQ(USART2_IRQn);

    uint32_t ack_mask = (0x1U << USART_ISR_TEACK_Pos) | (0x1U << USART_ISR_REACK_Pos);
    while ((USART2->ISR & ack_mask) != ack_mask) {}
    
    USART2->ICR = USART_ICR_IDLECF_Msk;

    rs485_rx_dma_start();
    
    return 1U;
}


uint8_t rs485_dma_write(const uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length == 0U) || (length > RS485_TX_BUFFER_SIZE))
    {
        return 0U;
    }

    if ((LPDMA2_CH0->CCR & DMA_CCR_EN_Msk) != 0U)
    {
        return 0U;
    }

    rs485_tx_dma_error = 0U;

    for (uint16_t i = 0; i < length; i++)
    {
        rs485_tx_buffer[i] = data[i];
    }

    USART2->ICR = USART_ICR_TCCF_Msk;

    rs485_de_high();

    LPDMA2_CH0->CSAR = (uint32_t)rs485_tx_buffer;
    LPDMA2_CH0->CDAR = (uint32_t)&USART2->TDR;
    LPDMA2_CH0->CBR1 = length;

    LPDMA2_CH0->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    LPDMA2_CH0->CCR |= DMA_CCR_EN_Msk;

    return 1U;
}

void rs485_process(void)
{
    if (rs485_tx_event != 0U)
    {
        rs485_tx_event = 0U;
        rs485_tx_callback((rs485_tx_dma_error == 0U) ? 1U : 0U);
    }

    if (rs485_rx_event == 0U)
    {
        return;
    }

    rs485_rx_event = 0U;

    LPDMA2_CH1->CCR |= DMA_CCR_SUSP_Msk;

    while ((LPDMA2_CH1->CSR & DMA_CSR_SUSPF_Msk) == 0U){}
    while ((LPDMA2_CH1->CSR & DMA_CSR_IDLEF_Msk) == 0U){}

    uint16_t remaining = (uint16_t)((LPDMA2_CH1->CBR1 & DMA_CBR1_BNDT_Msk) >> DMA_CBR1_BNDT_Pos);
    uint16_t received = (uint16_t)(RS485_RX_BUFFER_SIZE - remaining);

    LPDMA2_CH1->CCR |= DMA_CCR_RESET_Msk;

    while ((LPDMA2_CH1->CCR & DMA_CCR_EN_Msk) != 0U){}

    if ((received > 0U) && (rs485_rx_dma_error == 0U))
    {
        rs485_rx_callback((const uint8_t *)rs485_rx_dma_buffer, received);
    }
    rs485_rx_dma_start();
}

void USART2_IRQHandler(void)
{
    uint32_t status = USART2->ISR;

    if ((status & USART_ISR_IDLE_Msk) != 0U)
    {
        USART2->ICR = USART_ICR_IDLECF_Msk;
        rs485_rx_event = 1U;
    }

    if (((status & USART_ISR_TC_Msk) != 0U) && ((USART2->CR1 & USART_CR1_TCIE_Msk) != 0U))
    {
        USART2->ICR = USART_ICR_TCCF_Msk;
        USART2->CR1 &= ~USART_CR1_TCIE_Msk;
        rs485_de_low();
        rs485_tx_event = 1U;
    }
}

void LPDMA2_CH0_IRQHandler(void)
{
    uint32_t status = LPDMA2_CH0->CSR;

    if ((status & DMA_CSR_DTEF_Msk) != 0U)
    {
        LPDMA2_CH0->CFCR = DMA_CFCR_DTEF_Msk;
        rs485_tx_dma_error = 1U;
        rs485_de_low();
        rs485_tx_event = 1U;
        return;
    }

    if ((status & DMA_CSR_TCF_Msk) != 0U)
    {
        LPDMA2_CH0->CFCR = DMA_CFCR_TCF_Msk;
        USART2->CR1 |= USART_CR1_TCIE_Msk;
    }
}

void LPDMA2_CH1_IRQHandler(void)
{
    uint32_t status = LPDMA2_CH1->CSR;

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        LPDMA2_CH1->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        rs485_rx_dma_error = 1U;
    }
}