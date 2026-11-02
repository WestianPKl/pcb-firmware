#include "dma.h"

void dma1_init(void)
{
    RCC->AHBENR |= RCC_AHBENR_DMA1EN;
    (void)RCC->AHBENR;

    DMAMUX1_Channel0->CCR &= ~DMAMUX_CxCR_DMAREQ_ID_Msk;
    DMAMUX1_Channel0->CCR |= (5U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    DMAMUX1_Channel1->CCR &= ~DMAMUX_CxCR_DMAREQ_ID_Msk;
    DMAMUX1_Channel1->CCR |= (50U << DMAMUX_CxCR_DMAREQ_ID_Pos);

    DMAMUX1_Channel2->CCR &= ~DMAMUX_CxCR_DMAREQ_ID_Msk;
    DMAMUX1_Channel2->CCR |= (51U << DMAMUX_CxCR_DMAREQ_ID_Pos);
}