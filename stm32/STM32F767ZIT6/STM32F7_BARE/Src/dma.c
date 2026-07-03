#include "dma.h"
#include "support.h"
#include <stdio.h>

volatile uint8_t uart3_tx_busy;
volatile uint8_t uart2_tx_busy;

volatile uint8_t spi1_dma_rx_done = 0;
volatile uint8_t spi1_dma_tx_done = 0;

volatile uint8_t spi3_dma_rx_done = 0;
volatile uint8_t spi3_dma_tx_done = 0;

// DMA2 Stream0 Channel 0  ADC1

// DMA1 Stream1 Channel 1  USART3_RX
// DMA1 Stream3 Channel 4  USART3_TX

// DMA1 Stream5 Channel 4  USART2_RX
// DMA1 Stream6 Channel 4  USART2_TX

// DMA2 Stream2 Channel3 SPI1_RX
// DMA2 Stream3 Channel3 SPI1_TX

// DMA1 Stream0 Channel0 SPI3_RX
// DMA1 Stream7 Channel0 SPI3_TX

// DMA 1 Stream2 Channel7 I2C2_Rx
// DMA 1 Stream4 Channel7 I2C2_Tx

static int dma_stream_disable_timeout(DMA_Stream_TypeDef *DMAx)
{
    uint32_t t = 1000000U;
    DMAx->CR &= ~DMA_SxCR_EN;
    while ((DMAx->CR & DMA_SxCR_EN) != 0U) {
        if (--t == 0U) return -1;
    }
    return 0;
}

static void dma_stream_disable(DMA_Stream_TypeDef *DMAx)
{
    int rc = dma_stream_disable_timeout(DMAx);
    (void)rc;
}

static void dma_stream_config(DMA_Stream_TypeDef *DMAx, uint32_t chsel, uint32_t dir)
{
    dma_stream_disable(DMAx);
    DMAx->CR = 0;
    DMAx->CR |= (chsel << DMA_SxCR_CHSEL_Pos);
    DMAx->CR |= (dir   << DMA_SxCR_DIR_Pos);
    DMAx->CR |= DMA_SxCR_MINC;
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

void adc1_dma2_config(uint8_t continuous, uint16_t *dst, uint16_t len)
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
    if (continuous) {
        DMA2_Stream0->CR |= DMA_SxCR_CIRC;
    } else {
        DMA2_Stream0->CR |= DMA_SxCR_TCIE;
        NVIC_EnableIRQ(DMA2_Stream0_IRQn);
    }

    DMA2_Stream0->FCR = 0;

    DMA2_Stream0->CR |= DMA_SxCR_EN;
}

void uart3_dma1_rx_start(uint8_t *dst, uint16_t len)
{
    dma_stream_disable(DMA1_Stream1);

    DMA1_Stream1->PAR  = (uint32_t)&USART3->RDR;
    DMA1_Stream1->M0AR = (uint32_t)dst;
    DMA1_Stream1->NDTR = len;

    DMA1_Stream1->CR = 0;
    DMA1_Stream1->CR |= (1U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream1->CR |= (0x0U << DMA_SxCR_DIR_Pos);
    DMA1_Stream1->CR |= DMA_SxCR_MINC | DMA_SxCR_CIRC;
    DMA1_Stream1->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA1_Stream1->FCR = 0;
    
    DMA1_Stream1->CR |= DMA_SxCR_EN;
    
}

void uart3_dma1_tx_start(uint8_t *mem_addr, uint16_t len)
{
    dma_stream_disable(DMA1_Stream3);

    DMA1_Stream3->PAR  = (uint32_t)&USART3->TDR;
    DMA1_Stream3->M0AR = (uint32_t)mem_addr;
    DMA1_Stream3->NDTR = len;

    DMA1_Stream3->CR = 0;
    DMA1_Stream3->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream3->CR |= (0x1U << DMA_SxCR_DIR_Pos);
    DMA1_Stream3->CR |= DMA_SxCR_MINC | DMA_SxCR_TCIE;
    DMA1_Stream3->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA1_Stream3->FCR = 0;


    DMA1->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3
                | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3;

    NVIC_EnableIRQ(DMA1_Stream3_IRQn);

    DMA1_Stream3->CR |= DMA_SxCR_EN;

}

uint8_t uart3_write_dma(uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return 0;

    if (uart3_tx_busy) {
        return 0;
    }

    uart3_tx_busy = 1;
    uart3_dma1_tx_start(data, len);
    return 1;
}

void uart3_receive_dma(uint8_t *dst, uint16_t len)
{
    if (!dst || len == 0) return;

    uart3_dma1_rx_start(dst, len);
}

void uart2_dma1_rx_start(uint8_t *dst, uint16_t len)
{
    dma_stream_disable(DMA1_Stream5);

    DMA1_Stream5->PAR  = (uint32_t)&USART2->RDR;
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

void uart2_dma1_tx_start(uint8_t *mem_addr, uint16_t len)
{
    dma_stream_disable(DMA1_Stream6);

    DMA1_Stream6->PAR  = (uint32_t)&USART2->TDR;
    DMA1_Stream6->M0AR = (uint32_t)mem_addr;
    DMA1_Stream6->NDTR = len;

    DMA1_Stream6->CR = 0;
    DMA1_Stream6->CR |= (4U << DMA_SxCR_CHSEL_Pos);
    DMA1_Stream6->CR |= (0x1U << DMA_SxCR_DIR_Pos);
    DMA1_Stream6->CR |= DMA_SxCR_MINC | DMA_SxCR_TCIE;
    DMA1_Stream6->CR |= (0x0U << DMA_SxCR_MSIZE_Pos) | (0x0U << DMA_SxCR_PSIZE_Pos);

    DMA1_Stream6->FCR = 0;

    DMA1->HIFCR = DMA_HIFCR_CTCIF6 | DMA_HIFCR_CHTIF6 | DMA_HIFCR_CTEIF6
                | DMA_HIFCR_CDMEIF6 | DMA_HIFCR_CFEIF6;

    NVIC_SetPriority(DMA1_Stream6_IRQn, 5);
    NVIC_EnableIRQ(DMA1_Stream6_IRQn);

    DMA1_Stream6->CR |= DMA_SxCR_EN;

}

uint8_t uart2_write_dma(uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return 0;

    if (uart2_tx_busy) {
        return 0;
    }

    uart2_tx_busy = 1;
    uart2_dma1_tx_start(data, len);
    return 1;
}

void uart2_receive_dma(uint8_t *dst, uint16_t len)
{
    if (!dst || len == 0) return;

    uart2_dma1_rx_start(dst, len);
}

void spi1_dma_rx_init(void){
    dma_stream_config(DMA2_Stream2, 3U, 0U);
    DMA2_Stream2->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    NVIC_EnableIRQ(DMA2_Stream2_IRQn);
}

void spi1_dma_tx_init(void){
    dma_stream_config(DMA2_Stream3, 3U, 1U);
    DMA2_Stream3->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    NVIC_EnableIRQ(DMA2_Stream3_IRQn);
}

void spi1_dma_transfer(uint32_t msg_to_send, uint32_t msg_len){
    dma_stream_disable(DMA2_Stream3);
    DMA2->LIFCR = DMA_LIFCR_CTCIF3 | DMA_LIFCR_CHTIF3 | DMA_LIFCR_CTEIF3 | DMA_LIFCR_CDMEIF3 | DMA_LIFCR_CFEIF3;

    DMA2_Stream3->PAR  = (uint32_t)&SPI1->DR;
    DMA2_Stream3->M0AR = (uint32_t)msg_to_send;
    DMA2_Stream3->NDTR = msg_len;

    DMA2_Stream3->CR |= DMA_SxCR_EN;
}

void spi1_dma_receive(uint32_t received_msg, uint32_t msg_len){
    dma_stream_disable(DMA2_Stream2);
    DMA2->LIFCR = DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CFEIF2;

    DMA2_Stream2->PAR  = (uint32_t)&SPI1->DR;
    DMA2_Stream2->M0AR = (uint32_t)received_msg;
    DMA2_Stream2->NDTR = msg_len;

    DMA2_Stream2->CR |= DMA_SxCR_EN;
}


void spi3_dma_rx_init(void){
    dma_stream_config(DMA1_Stream0, 0U, 0U);
    DMA1_Stream0->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    NVIC_EnableIRQ(DMA1_Stream0_IRQn);
}

void spi3_dma_tx_init(void){
    dma_stream_config(DMA1_Stream7, 0U, 1U);
    DMA1_Stream7->CR |= DMA_SxCR_TCIE | DMA_SxCR_TEIE;
    NVIC_EnableIRQ(DMA1_Stream7_IRQn);
}

void spi3_dma_transfer(uint32_t msg_to_send, uint32_t msg_len){
    dma_stream_disable(DMA1_Stream7);
    DMA1->HIFCR = DMA_HIFCR_CTCIF7 | DMA_HIFCR_CHTIF7 | DMA_HIFCR_CTEIF7 | DMA_HIFCR_CDMEIF7 | DMA_HIFCR_CFEIF7;

    DMA1_Stream7->PAR  = (uint32_t)&SPI3->DR;
    DMA1_Stream7->M0AR = (uint32_t)msg_to_send;
    DMA1_Stream7->NDTR = msg_len;

    DMA1_Stream7->CR |= DMA_SxCR_EN;
}

void spi3_dma_receive(uint32_t received_msg, uint32_t msg_len){
    dma_stream_disable(DMA1_Stream0);
    DMA1->LIFCR = DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CTEIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;

    DMA1_Stream0->PAR  = (uint32_t)&SPI3->DR;
    DMA1_Stream0->M0AR = (uint32_t)received_msg;
    DMA1_Stream0->NDTR = msg_len;

    DMA1_Stream0->CR |= DMA_SxCR_EN;
}

static void spi1_dma_wait_done(void)
{
    uint32_t t = SPI_DMA_TIMEOUT;
    while (!(spi1_dma_rx_done && spi1_dma_tx_done)) {
        if (--t == 0U) break;
    }
    spi1_dma_rx_done = 0;
    spi1_dma_tx_done = 0;
}

void spi1_dma_xfer(const uint8_t *tx, uint8_t *rx, uint32_t len)
{
    static uint8_t dummy_tx = 0xFFU;
    static uint8_t sink_rx;

    spi1_dma_rx_done = 0;
    spi1_dma_tx_done = 0;

    if (rx) DMA2_Stream2->CR |= DMA_SxCR_MINC;
    else    DMA2_Stream2->CR &= ~DMA_SxCR_MINC;
    spi1_dma_receive((uint32_t)(rx ? rx : &sink_rx), len);

    if (tx) DMA2_Stream3->CR |= DMA_SxCR_MINC;
    else    DMA2_Stream3->CR &= ~DMA_SxCR_MINC;
    spi1_dma_transfer((uint32_t)(tx ? tx : &dummy_tx), len);

    spi1_dma_wait_done();

    DMA2_Stream2->CR |= DMA_SxCR_MINC;
    DMA2_Stream3->CR |= DMA_SxCR_MINC;
}

void i2c2_dma_tx_init(void){
    dma_stream_config(DMA1_Stream4, 7U, 1U);
}

void i2c2_dma_rx_init(void){
    dma_stream_config(DMA1_Stream2, 7U, 0U);
}

void i2c2_dma_tx_start(uint32_t buf, uint16_t len){
    dma_stream_disable(DMA1_Stream4);
    DMA1->HIFCR = DMA_HIFCR_CTCIF4 | DMA_HIFCR_CHTIF4 | DMA_HIFCR_CTEIF4 | DMA_HIFCR_CDMEIF4 | DMA_HIFCR_CFEIF4;

    DMA1_Stream4->PAR  = (uint32_t)&I2C2->TXDR;
    DMA1_Stream4->M0AR = buf;
    DMA1_Stream4->NDTR = len;

    DMA1_Stream4->CR |= DMA_SxCR_EN;
}

void i2c2_dma_rx_start(uint32_t buf, uint16_t len){
    dma_stream_disable(DMA1_Stream2);
    DMA1->LIFCR = DMA_LIFCR_CTCIF2 | DMA_LIFCR_CHTIF2 | DMA_LIFCR_CTEIF2 | DMA_LIFCR_CDMEIF2 | DMA_LIFCR_CFEIF2;

    DMA1_Stream2->PAR  = (uint32_t)&I2C2->RXDR;
    DMA1_Stream2->M0AR = buf;
    DMA1_Stream2->NDTR = len;

    DMA1_Stream2->CR |= DMA_SxCR_EN;
}


void DMA2_Stream0_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;
    }
}

void DMA2_Stream2_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF2) {
        spi1_dma_rx_done = 1;
        DMA2->LIFCR = DMA_LIFCR_CTCIF2;
    }
    if (DMA2->LISR & DMA_LISR_TEIF2) {
        DMA2->LIFCR = DMA_LIFCR_CTEIF2;
    }
}

void DMA2_Stream3_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF3) {
        spi1_dma_tx_done = 1;
        DMA2->LIFCR = DMA_LIFCR_CTCIF3;
    }
    if (DMA2->LISR & DMA_LISR_TEIF3) {
        DMA2->LIFCR = DMA_LIFCR_CTEIF3;
    }
}

void DMA1_Stream0_IRQHandler(void)
{
    if (DMA1->LISR & DMA_LISR_TCIF0) {
        spi3_dma_rx_done = 1;
        DMA1->LIFCR = DMA_LIFCR_CTCIF0;
    }
    if (DMA1->LISR & DMA_LISR_TEIF0) {
        DMA1->LIFCR = DMA_LIFCR_CTEIF0;
    }
}

void DMA1_Stream7_IRQHandler(void)
{
    if (DMA1->HISR & DMA_HISR_TCIF7) {
        spi3_dma_tx_done = 1;
        DMA1->HIFCR = DMA_HIFCR_CTCIF7;
    }
    if (DMA1->HISR & DMA_HISR_TEIF7) {
        DMA1->HIFCR = DMA_HIFCR_CTEIF7;
    }
}

void DMA1_Stream3_IRQHandler(void)
{
    if (DMA1->LISR & DMA_LISR_TCIF3) {
        DMA1->LIFCR = DMA_LIFCR_CTCIF3;
        uart3_tx_busy = 0;
    }
}

void DMA1_Stream6_IRQHandler(void)
{
    if (DMA1->HISR & DMA_HISR_TCIF6) {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        uart2_tx_busy = 0;
    }
}

