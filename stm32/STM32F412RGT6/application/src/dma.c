#include "dma.h"

static int dma_stream_disable_timeout(DMA_Stream_TypeDef *s)
{
    uint32_t t = 1000000U;
    s->CR &= ~DMA_SxCR_EN;
    while ((s->CR & DMA_SxCR_EN) != 0U) {
        if (--t == 0U) return -1;
    }
    return 0;
}

static void dma_stream_disable(DMA_Stream_TypeDef *s)
{
    int rc = dma_stream_disable_timeout(s);
    (void)rc;
}

void dma1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    (void)RCC->AHB1ENR;
}

void dma2_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA2EN;
    (void)RCC->AHB1ENR;
}

void dma2_adc1_config(uint8_t circular, uint16_t *dst, uint16_t len)
{
    dma_stream_disable(DMA2_Stream0);

    DMA2_Stream0->PAR  = (uint32_t)&ADC1->DR;
    DMA2_Stream0->M0AR = (uint32_t)dst;
    DMA2_Stream0->NDTR = len;

    DMA2_Stream0->CR = 0;
    DMA2_Stream0->CR |= (0U << DMA_SxCR_CHSEL_Pos);
    DMA2_Stream0->CR |= (0x0U << DMA_SxCR_DIR_Pos);
    DMA2_Stream0->CR |= DMA_SxCR_MINC;
    DMA2_Stream0->CR |= (0x1U << DMA_SxCR_MSIZE_Pos); 
    DMA2_Stream0->CR |= (0x1U << DMA_SxCR_PSIZE_Pos);
    DMA2_Stream0->CR |= DMA_SxCR_TCIE;

    if (circular) DMA2_Stream0->CR |= DMA_SxCR_CIRC;

    DMA2_Stream0->FCR = 0;

    NVIC_EnableIRQ(DMA2_Stream0_IRQn);

    DMA2_Stream0->CR |= DMA_SxCR_EN;
}

void dma1_uart2_rx_config(uint8_t *dst, uint16_t len)
{
    dma_stream_disable(DMA1_Stream5);

    DMA1_Stream5->PAR  = (uint32_t)&USART2->DR;
    DMA1_Stream5->M0AR = (uint32_t)dst;
    DMA1_Stream5->NDTR = len;

    DMA1_Stream5->CR = 0;
    DMA1_Stream5->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream5->CR |= (0x0U << DMA_SxCR_DIR_Pos);
    DMA1_Stream5->CR |= DMA_SxCR_MINC | DMA_SxCR_CIRC;
    DMA1_Stream5->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA1_Stream5->FCR = 0;
    
    DMA1_Stream5->CR |= DMA_SxCR_EN;
}

void dma1_uart2_tx_start(uint8_t *mem_addr, uint16_t len)
{
    dma_stream_disable(DMA1_Stream6);

    DMA1_Stream6->PAR  = (uint32_t)&USART2->DR;
    DMA1_Stream6->M0AR = (uint32_t)mem_addr;
    DMA1_Stream6->NDTR = len;

    DMA1_Stream6->CR = 0;
    DMA1_Stream6->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream6->CR |= (0x1U << DMA_SxCR_DIR_Pos);
    DMA1_Stream6->CR |= DMA_SxCR_MINC | DMA_SxCR_TCIE;
    DMA1_Stream6->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA1_Stream6->FCR = 0;

    NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    DMA1_Stream6->CR |= DMA_SxCR_EN;
}

void dma2_uart1_rx_config(uint8_t *dst, uint16_t len)
{
    dma_stream_disable(DMA2_Stream5);

    DMA2_Stream5->PAR  = (uint32_t)&USART1->DR;
    DMA2_Stream5->M0AR = (uint32_t)dst;
    DMA2_Stream5->NDTR = len;

    DMA2_Stream5->CR = 0;
    DMA2_Stream5->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA2_Stream5->CR |= (0x0U << DMA_SxCR_DIR_Pos);
    DMA2_Stream5->CR |= DMA_SxCR_MINC | DMA_SxCR_CIRC;
    DMA2_Stream5->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA2_Stream5->FCR = 0;

    DMA2_Stream5->CR |= DMA_SxCR_EN;
}

void dma2_uart1_tx_start(uint8_t *mem_addr, uint16_t len)
{
    dma_stream_disable(DMA2_Stream7);

    DMA2_Stream7->PAR  = (uint32_t)&USART1->DR;
    DMA2_Stream7->M0AR = (uint32_t)mem_addr;
    DMA2_Stream7->NDTR = len;

    DMA2_Stream7->CR = 0;
    DMA2_Stream7->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA2_Stream7->CR |= (0x1U << DMA_SxCR_DIR_Pos);
    DMA2_Stream7->CR |= DMA_SxCR_MINC | DMA_SxCR_TCIE;
    DMA2_Stream7->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA2_Stream7->FCR = 0;

    NVIC_EnableIRQ(DMA2_Stream7_IRQn);

    DMA2_Stream7->CR |= DMA_SxCR_EN;
}

void dma_i2c1_rx_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    (void)RCC->AHB1ENR;

    dma_stream_disable(DMA1_Stream0);

    DMA1->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;

    DMA1_Stream0->CR = 0;
    DMA1_Stream0->CR |= (1U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream0->CR |= (0U << DMA_SxCR_DIR_Pos);
    DMA1_Stream0->CR |= DMA_SxCR_MINC;
    DMA1_Stream0->CR |= (0U << DMA_SxCR_MSIZE_Pos);
    DMA1_Stream0->CR |= (0U << DMA_SxCR_PSIZE_Pos);
    DMA1_Stream0->CR |= (2U << DMA_SxCR_PL_Pos);
    DMA1_Stream0->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;

    DMA1_Stream0->FCR = 0;

    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void dma_i2c1_tx_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    (void)RCC->AHB1ENR;

    dma_stream_disable(DMA1_Stream1);

    DMA1->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

    DMA1_Stream1->CR = 0;
    DMA1_Stream1->CR |= (0U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream1->CR |= (1U << DMA_SxCR_DIR_Pos); 
    DMA1_Stream1->CR |= DMA_SxCR_MINC;
    DMA1_Stream1->CR |= (0U << DMA_SxCR_MSIZE_Pos);
    DMA1_Stream1->CR |= (0U << DMA_SxCR_PSIZE_Pos);
    DMA1_Stream1->CR |= (2U << DMA_SxCR_PL_Pos);
    DMA1_Stream1->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;

    DMA1_Stream1->FCR = 0;

    NVIC_EnableIRQ(DMA1_Stream1_IRQn);
}

void dma_i2c1_rx_start(uint32_t dst, uint16_t len)
{
    if (len == 0U) return;

    dma_stream_disable(DMA1_Stream0);

    DMA1->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;

    DMA1_Stream0->PAR  = (uint32_t)&I2C1->DR;
    DMA1_Stream0->M0AR = dst;
    DMA1_Stream0->NDTR = len;

    DMA1_Stream0->CR |= DMA_SxCR_EN;
}

void dma_i2c1_tx_start(uint32_t src, uint16_t len)
{
    if (len == 0U) return;

    dma_stream_disable(DMA1_Stream1);

    DMA1->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;

    DMA1_Stream1->PAR  = (uint32_t)&I2C1->DR;
    DMA1_Stream1->M0AR = src;
    DMA1_Stream1->NDTR = len;

    DMA1_Stream1->CR |= DMA_SxCR_EN;
}

void dma_i2c1_abort(void)
{
    dma_stream_disable(DMA1_Stream0);
    DMA1->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;
    dma_stream_disable(DMA1_Stream1);
    DMA1->LIFCR = DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CTEIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;
}

void dma_spi1_rx_init(void){
    dma_stream_disable(DMA2_Stream2);

    DMA2_Stream2->CR = 0;
    DMA2_Stream2->CR |= (3U << DMA_SxCR_CHSEL_Pos);
    DMA2_Stream2->CR |= (0U << DMA_SxCR_DIR_Pos);
    DMA2_Stream2->CR |= DMA_SxCR_MINC;
    DMA2_Stream2->CR &= ~DMA_SxCR_PINC;
    DMA2_Stream2->CR &= ~DMA_SxCR_CIRC;

    DMA2_Stream2->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}

void dma_spi1_tx_init(void){
    dma_stream_disable(DMA2_Stream3);

    DMA2_Stream3->CR = 0;
    DMA2_Stream3->CR |= (3U << DMA_SxCR_CHSEL_Pos);
    DMA2_Stream3->CR |= (1U << DMA_SxCR_DIR_Pos);
    DMA2_Stream3->CR |= DMA_SxCR_MINC;
    DMA2_Stream3->CR &= ~DMA_SxCR_PINC;
    DMA2_Stream3->CR &= ~DMA_SxCR_CIRC;
    
    DMA2_Stream3->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE; 
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

void dma_spi1_transfer(uint32_t msg_to_send, uint32_t msg_len){
    dma_stream_disable(DMA2_Stream3);
    DMA2->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3;

    DMA2_Stream3->PAR  = (uint32_t)&SPI1->DR;
    DMA2_Stream3->M0AR = (uint32_t)msg_to_send;
    DMA2_Stream3->NDTR = msg_len;

    DMA2_Stream3->CR |= DMA_SxCR_EN;
}

void dma_spi1_receive(uint32_t received_msg, uint32_t msg_len){
    dma_stream_disable(DMA2_Stream2);
    DMA2->LIFCR = DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CFEIF2;

    DMA2_Stream2->PAR  = (uint32_t)&SPI1->DR;
    DMA2_Stream2->M0AR = (uint32_t)received_msg;
    DMA2_Stream2->NDTR = msg_len;

    DMA2_Stream2->CR |= DMA_SxCR_EN;
}