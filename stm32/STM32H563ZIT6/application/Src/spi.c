#include "spi.h"

volatile uint8_t spi1_dma_busy = 0U;
volatile uint8_t spi1_dma_done = 0U;
volatile uint8_t spi1_dma_error = 0U;
volatile uint8_t spi1_cs_bme = 0U;

#define SPI2_FRAME_SIZE 4U

volatile uint8_t spi2_frame_ready = 0U;
volatile uint8_t spi2_frame_overrun = 0U;

volatile uint8_t spi2_rx_work[SPI2_FRAME_SIZE];
volatile uint8_t spi2_rx_frame[SPI2_FRAME_SIZE];

volatile uint8_t spi2_rx_pos = 0U;
volatile uint8_t spi2_rearm_pending = 0U;

static const uint8_t spi2_tx[SPI2_FRAME_SIZE] =
{
    0x11, 0x22, 0x33, 0x44
};

static inline void spi1_cs_high(void)
{
    GPIOF->BSRR = (1U << SPI1_CS);
}

static inline void spi1_cs_low(void)
{
    GPIOF->BSRR = (1U << (SPI1_CS + 16));
}

static inline void spi1_cs_bme_high(void)
{
    GPIOE->BSRR = (1U << SPI1_CS_BME);
}

static inline void spi1_cs_bme_low(void)
{
    GPIOE->BSRR = (1U << (SPI1_CS_BME + 16));
}

static void spi1_tx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel4->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    GPDMA1_Channel4->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel4->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

    GPDMA1_Channel4->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel4->CTR2 |= (0x7U << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_DREQ_Pos);

    GPDMA1_Channel4->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel4->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC->IPR[31] = 0x50U;
    NVIC->ICPR[0] = (1U << 31U);
    NVIC->ISER[0] = (1U << 31U);

    SPI1->CFG1 |= (0x1U << SPI_CFG1_TXDMAEN_Pos);
}

static void spi1_rx_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel5->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

    GPDMA1_Channel5->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel5->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

    GPDMA1_Channel5->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_DREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel5->CTR2 |= (0x6U << DMA_CTR2_REQSEL_Pos);

    GPDMA1_Channel5->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
    GPDMA1_Channel5->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

    NVIC->IPR[32] = 0x50U;
    NVIC->ICPR[1] = (1U << 0U);
    NVIC->ISER[1] = (1U << 0U);

    SPI1->CFG1 |= (0x1U << SPI_CFG1_RXDMAEN_Pos);
}

void spi1_init(uint8_t dma_enable)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIODEN  | RCC_AHB2ENR_GPIOEEN | RCC_AHB2ENR_GPIOFEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER &= ~(0x3U << (SPI1_SCK * 2U) | 0x3U << (SPI1_MISO * 2U));
    GPIOA->MODER |= (0x2U << (SPI1_SCK * 2U) | 0x2U << (SPI1_MISO * 2U));
    GPIOA->OTYPER &= ~((0x1U << SPI1_SCK) | (0x1U << SPI1_MISO));
    GPIOA->OSPEEDR |= (0x3U << (SPI1_SCK * 2U) | 0x3U << (SPI1_MISO * 2U));
    GPIOA->PUPDR &= ~(0x3U << (SPI1_SCK * 2U) | 0x3U << (SPI1_MISO * 2U));

    GPIOD->MODER &= ~(0x3U << (SPI1_MOSI * 2U));
    GPIOD->MODER |= (0x2U << (SPI1_MOSI * 2U));
    GPIOD->OTYPER &= ~(0x1U << SPI1_MOSI);
    GPIOD->OSPEEDR |= (0x3U << (SPI1_MOSI * 2U));
    GPIOD->PUPDR &= ~(0x3U << (SPI1_MOSI * 2U));

    GPIOF->MODER &= ~(0x3U << (SPI1_CS * 2U));
    GPIOF->MODER |= (0x1U << (SPI1_CS * 2U));
    GPIOF->OTYPER &= ~(0x1U << SPI1_CS);
    GPIOF->OSPEEDR |= (0x3U << (SPI1_CS * 2U));
    GPIOF->PUPDR &= ~(0x3U << (SPI1_CS * 2U));
    GPIOF->BSRR = (0x1U << SPI1_CS);

    GPIOE->MODER &= ~(0x3U << (SPI1_CS_BME * 2U));
    GPIOE->MODER |= (0x1U << (SPI1_CS_BME * 2U));
    GPIOE->OTYPER &= ~(0x1U << SPI1_CS_BME);
    GPIOE->OSPEEDR |= (0x3U << (SPI1_CS_BME * 2U));
    GPIOE->PUPDR &= ~(0x3U << (SPI1_CS_BME * 2U));
    GPIOE->BSRR = (0x1U << SPI1_CS_BME);

    GPIOA->AFR[0] &= ~((0xFU << (SPI1_SCK * 4U)) | (0xFU << (SPI1_MISO * 4U)));
    GPIOA->AFR[0] |= (0x5U << (SPI1_SCK * 4U)) | (0x5U << (SPI1_MISO * 4U));

    GPIOD->AFR[0] &= ~(0xFU << (SPI1_MOSI * 4U));
    GPIOD->AFR[0] |=  (0x5U << (SPI1_MOSI * 4U));

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);
    SPI1->CR1 |= SPI_CR1_SSI_Msk;

    SPI1->CFG1 &= ~(SPI_CFG1_DSIZE_Msk | SPI_CFG1_FTHLV_Msk | SPI_CFG1_MBR_Msk);
    SPI1->CFG1 |= (0x7U << SPI_CFG1_DSIZE_Pos) | (0x6U << SPI_CFG1_MBR_Pos);

    SPI1->CFG2 &= ~((SPI_CFG2_COMM_Msk) | (SPI_CFG2_SP_Msk) | (0x1U << SPI_CFG2_LSBFRST_Pos) | (0x1U << SPI_CFG2_CPHA_Pos) | (0x1U << SPI_CFG2_CPOL_Pos));

    SPI1->CFG2 |= SPI_CFG2_SSM_Msk;
    SPI1->IFCR = SPI_IFCR_MODFC_Msk;
    SPI1->CFG2 |= SPI_CFG2_MASTER_Msk;

    if (dma_enable)
    {
        SPI1->IER &= ~SPI_IER_EOTIE_Msk;
        SPI1->IFCR = SPI_IFCR_EOTC_Msk;

        spi1_tx_dma_init();
        spi1_rx_dma_init();

        NVIC->IPR[55] = 0x50U;
        NVIC->ICPR[1] = (1U << 23U);
        NVIC->ISER[1] = (1U << 23U);
    }
}

static void spi1_dma_abort(void)
{
    SPI1->IER &= ~SPI_IER_EOTIE_Msk;

    SPI1->CR1 &= ~SPI_CR1_SPE_Msk;

    GPDMA1_Channel4->CCR &= ~DMA_CCR_EN_Msk;
    GPDMA1_Channel5->CCR &= ~DMA_CCR_EN_Msk;

    if (spi1_cs_bme != 0U)
    {
        spi1_cs_bme_high();
    }
    else
    {
        spi1_cs_high();
    }

    spi1_dma_done  = 0U;
    spi1_dma_error = 1U;
    spi1_dma_busy  = 0U;
}

static void spi2_arm(void)
{
    SPI2->CR1 &= ~SPI_CR1_SPE_Msk;

    SPI2->CR2 &= ~SPI_CR2_TSIZE_Msk;
    SPI2->CR2 |= (SPI2_FRAME_SIZE << SPI_CR2_TSIZE_Pos);

    SPI2->IFCR = SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk | SPI_IFCR_OVRC_Msk | SPI_IFCR_UDRC_Msk;

    spi2_rx_pos = 0U;

    SPI2->CR1 |= SPI_CR1_SPE_Msk;

    for (uint32_t i = 0U; i < SPI2_FRAME_SIZE; i++)
    {
        while ((SPI2->SR & SPI_SR_TXP_Msk) == 0U)
        {
        }

        *(volatile uint8_t *)&SPI2->TXDR = spi2_tx[i];
    }
}

void spi2_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;

    GPIOB->MODER &= ~(0x3U << (SPI2_SCK * 2U) | 0x3U << (SPI2_CS * 2U));
    GPIOB->MODER |= (0x2U << (SPI2_SCK * 2U) | 0x2U << (SPI2_CS * 2U));
    GPIOB->OTYPER &= ~((0x1U << SPI2_SCK) | (0x1U << SPI2_CS));
    GPIOB->OSPEEDR |= (0x3U << (SPI2_SCK * 2U) | 0x3U << (SPI2_CS * 2U));
    GPIOB->PUPDR &= ~(0x3U << (SPI2_SCK * 2U) | 0x3U << (SPI2_CS * 2U));
    GPIOB->PUPDR |=  (0x1U << (SPI2_CS * 2U));

    GPIOC->MODER &= ~(0x3U << (SPI2_MISO * 2U) | 0x3U << (SPI2_MOSI * 2U));
    GPIOC->MODER |= (0x2U << (SPI2_MISO * 2U) | 0x2U << (SPI2_MOSI * 2U));
    GPIOC->OTYPER &= ~(0x1U << SPI2_MISO | 0x1U << SPI2_MOSI);
    GPIOC->OSPEEDR |= (0x3U << (SPI2_MISO * 2U) | 0x3U << (SPI2_MOSI * 2U));
    GPIOC->PUPDR &= ~(0x3U << (SPI2_MISO * 2U) | 0x3U << (SPI2_MOSI * 2U));

    GPIOB->AFR[1] &= ~((0xFU << ((SPI2_SCK - 8U) * 4U)) | (0xFU << ((SPI2_CS - 8U) * 4U)));
    GPIOB->AFR[1] |= (0x5U << ((SPI2_SCK - 8U) * 4U)) | (0x5U << ((SPI2_CS - 8U) * 4U));

    GPIOC->AFR[0] &= ~(0xFU << (SPI2_MISO * 4U) | 0xFU << (SPI2_MOSI * 4U));
    GPIOC->AFR[0] |=  (0x5U << (SPI2_MISO * 4U) | 0x5U << (SPI2_MOSI * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_SPI2EN;
    (void)RCC->APB1LENR;

    SPI2->CR1 &= ~(SPI_CR1_SPE_Msk | SPI_CR1_SSI_Msk);

    SPI2->CR2 &= ~SPI_CR2_TSIZE_Msk;
    SPI2->CR2 |= (SPI2_FRAME_SIZE << SPI_CR2_TSIZE_Pos);

    SPI2->CFG2 &= ~SPI_CFG2_SSM_Msk;
    SPI2->CR1  &= ~SPI_CR1_SSI_Msk;

    SPI2->CFG1 &= ~(SPI_CFG1_DSIZE_Msk | SPI_CFG1_FTHLV_Msk);
    SPI2->CFG1 |= (0x7U << SPI_CFG1_DSIZE_Pos);

    SPI2->CFG2 &= ~(SPI_CFG2_COMM_Msk | SPI_CFG2_SP_Msk | SPI_CFG2_IOSWP_Msk | SPI_CFG2_LSBFRST_Msk | SPI_CFG2_CPHA_Msk | SPI_CFG2_CPOL_Msk | SPI_CFG2_SSM_Msk | SPI_CFG2_MASTER_Msk);
    SPI2->CFG2 &= ~SPI_CFG2_AFCNTR_Msk;

    SPI2->IER |= SPI_IER_RXPIE_Msk | SPI_IER_EOTIE_Msk;

    NVIC->IPR[56] = 0x50U;
    NVIC->ICPR[1] = (1U << 24U);
    NVIC->ISER[1] = (1U << 24U);

    SPI2->IFCR = SPI_IFCR_EOTC_Msk | SPI_IFCR_OVRC_Msk | SPI_IFCR_UDRC_Msk;

    spi2_arm();
}
void spi2_process(void)
{
    if (spi2_rearm_pending != 0U)
    {
        if ((GPIOB->IDR & (1U << SPI2_CS)) != 0U)
        {
            spi2_rearm_pending = 0U;
            spi2_arm();
        }
    }
}

uint8_t spi1_transfer_byte(uint8_t tx, uint8_t *rx)
{
    if (spi1_dma_busy != 0U)
    {
        return 0U;
    }

    SPI1->IFCR = (SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk | SPI_IFCR_OVRC_Msk);

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);
    SPI1->CR2 &= ~(SPI_CR2_TSIZE_Msk);
    SPI1->CR2 |= (1U << SPI_CR2_TSIZE_Pos);
    SPI1->CR1 |= (0x1U << SPI_CR1_SPE_Pos);

    spi1_cs_low();

    while ((SPI1->SR & SPI_SR_TXP_Msk) == 0U) {}
    *(volatile uint8_t *)&SPI1->TXDR = tx;

    SPI1->CR1 |= (1U << SPI_CR1_CSTART_Pos);

    while ((SPI1->SR & SPI_SR_RXP_Msk) == 0U) {}
    *rx = *(volatile uint8_t *)&SPI1->RXDR;
    while ((SPI1->SR & SPI_SR_EOT_Msk) == 0U) {}
    SPI1->IFCR = SPI_IFCR_EOTC_Msk;

    spi1_cs_high();
    return 1U;
}

uint8_t spi1_dma_transfer(const uint8_t *tx, uint8_t *rx, uint16_t length, uint8_t cs_bme)
{
    if ((tx == NULL) || (rx == NULL) || (length == 0U))
    {
        return 0U;
    }

    if (spi1_dma_busy != 0U)
    {
        return 0U;
    }

    if ((GPDMA1_Channel4->CCR & (1U << DMA_CCR_EN_Pos)) != 0U || (GPDMA1_Channel5->CCR & (1U << DMA_CCR_EN_Pos)) != 0U)
    {
        return 0U;
    }

    spi1_cs_bme = cs_bme;
    spi1_dma_busy = 1U;
    spi1_dma_done = 0U;
    spi1_dma_error = 0U;

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);

    SPI1->CR2 &= ~(SPI_CR2_TSIZE_Msk);
    SPI1->CR2 |= (length << SPI_CR2_TSIZE_Pos);

    SPI1->IFCR = (SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk | SPI_IFCR_OVRC_Msk);
    SPI1->IER |= SPI_IER_EOTIE_Msk;

    GPDMA1_Channel4->CCR &= ~(DMA_CCR_EN_Msk);
    GPDMA1_Channel5->CCR &= ~(DMA_CCR_EN_Msk);

    GPDMA1_Channel4->CSAR = (uint32_t)tx;
    GPDMA1_Channel4->CDAR = (uint32_t)&SPI1->TXDR;
    GPDMA1_Channel4->CBR1 = length;

    GPDMA1_Channel5->CSAR = (uint32_t)&SPI1->RXDR;
    GPDMA1_Channel5->CDAR = (uint32_t)rx;
    GPDMA1_Channel5->CBR1 = length;

    GPDMA1_Channel4->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;
    GPDMA1_Channel5->CFCR = DMA_CFCR_TCF_Msk | DMA_CFCR_DTEF_Msk;

    GPDMA1_Channel5->CCR |= (0x1U << DMA_CCR_EN_Pos);
    GPDMA1_Channel4->CCR |= (0x1U << DMA_CCR_EN_Pos);

    if (cs_bme != 0U)
    {
        spi1_cs_bme_low();
    }
    else
    {
        spi1_cs_low();
    }

    SPI1->CR1 |= (0x1U << SPI_CR1_SPE_Pos);
    SPI1->CR1 |= (1U << SPI_CR1_CSTART_Pos);

    return 1U;
}

uint8_t spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t length, uint8_t cs_bme)
{
    if (spi1_dma_busy != 0U)
    {
        return 0U;
    }

    if ((tx == NULL) || (rx == NULL) || (length == 0U))
    {
        return 0U;
    }

    spi1_cs_bme = cs_bme;
    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);
    SPI1->CR2 &= ~(SPI_CR2_TSIZE_Msk);
    SPI1->CR2 |= (length << SPI_CR2_TSIZE_Pos);
    SPI1->IFCR = (SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk | SPI_IFCR_OVRC_Msk);
    SPI1->CR1 |= (0x1U << SPI_CR1_SPE_Pos);

    if (cs_bme != 0U)
    {
        spi1_cs_bme_low();
    }
    else
    {
        spi1_cs_low();
    }

    while ((SPI1->SR & SPI_SR_TXP_Msk) == 0U) {}
    *(volatile uint8_t *)&SPI1->TXDR = tx[0];

    SPI1->CR1 |= SPI_CR1_CSTART_Msk;

    for (uint16_t i = 0U; i < length; i++)
    {
        if (i > 0U)
        {
            while ((SPI1->SR & SPI_SR_TXP_Msk) == 0U) {}
            *(volatile uint8_t *)&SPI1->TXDR = tx[i];
        }

        while ((SPI1->SR & SPI_SR_RXP_Msk) == 0U) {}
        rx[i] = *(volatile uint8_t *)&SPI1->RXDR;
    }

    while ((SPI1->SR & SPI_SR_EOT_Msk) == 0U) {}
    SPI1->IFCR = SPI_IFCR_EOTC_Msk;

    if (cs_bme != 0U)
    {
        spi1_cs_bme_high();
    }
    else
    {
        spi1_cs_high();
    }
    return 1U;
}

void SPI1_IRQHandler(void)
{
    uint32_t status = SPI1->SR;

    if ((status & SPI_SR_EOT_Msk) != 0U)
    {
        SPI1->IFCR = SPI_IFCR_EOTC_Msk;
        SPI1->IER &= ~SPI_IER_EOTIE_Msk;

        if (spi1_cs_bme != 0U)
        {
            spi1_cs_bme_high();
        }
        else
        {
            spi1_cs_high();
        }

        spi1_dma_busy = 0U;
        spi1_dma_done = 1U;
    }
}

void SPI2_IRQHandler(void)
{
    while ((SPI2->SR & SPI_SR_RXP_Msk) != 0U)
    {
        uint8_t data = *(volatile uint8_t *)&SPI2->RXDR;

        if (spi2_rx_pos < SPI2_FRAME_SIZE)
        {
            spi2_rx_work[spi2_rx_pos++] = data;
        }
    }

    if ((SPI2->SR & SPI_SR_EOT_Msk) != 0U)
    {
        SPI2->IFCR = SPI_IFCR_EOTC_Msk;

        if (spi2_rx_pos == SPI2_FRAME_SIZE)
        {
            if (spi2_frame_ready == 0U)
            {
                for (uint32_t i = 0U; i < SPI2_FRAME_SIZE; i++)
                {
                    spi2_rx_frame[i] = spi2_rx_work[i];
                }

                spi2_frame_ready = 1U;
            }
            else
            {
                spi2_frame_overrun = 1U;
            }
        }
        spi2_rearm_pending = 1U;
    }
}

void GPDMA1_CH4_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel4->CSR;

    if ((status & DMA_CSR_DTEF_Msk) != 0U)
    {
        GPDMA1_Channel4->CFCR = DMA_CFCR_DTEF_Msk;
        spi1_dma_abort();
    }
}

void GPDMA1_CH5_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel5->CSR;

    if ((status & DMA_CSR_DTEF_Msk) != 0U)
    {
        GPDMA1_Channel5->CFCR = DMA_CFCR_DTEF_Msk;
        spi1_dma_abort();
    }
}