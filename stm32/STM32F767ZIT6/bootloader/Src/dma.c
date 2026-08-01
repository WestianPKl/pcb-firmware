#include "dma.h"
#include "support.h"
#include <stdio.h>

volatile uint8_t uart3_tx_busy;
volatile uint8_t uart2_tx_busy;

// DMA1 Stream1 Channel 1  USART3_RX
// DMA1 Stream3 Channel 4  USART3_TX

// DMA1 Stream5 Channel 4  USART2_RX
// DMA1 Stream6 Channel 4  USART2_TX

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

void dma1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_DMA1EN;
    (void)RCC->AHB1ENR;
}

void dma1_stop_all(void)
{
    if (!(RCC->AHB1ENR & RCC_AHB1ENR_DMA1EN)) return;

    dma_stream_disable(DMA1_Stream1);
    dma_stream_disable(DMA1_Stream3);
    dma_stream_disable(DMA1_Stream5);
    dma_stream_disable(DMA1_Stream6);
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

