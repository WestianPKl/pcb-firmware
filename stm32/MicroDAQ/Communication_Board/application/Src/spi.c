#include "spi.h"
#include "output.h"
#include <stddef.h>

static spi_config_t current_config;

static inline void spi1_cs_high(void)
{
    output_on(GPIOA, SPI1_CS);
}

static inline void spi1_cs_low(void)
{
    output_off(GPIOA, SPI1_CS);
}

static uint8_t spi1_wait_status(uint32_t status)
{
    uint32_t timeout = 100000U;

    while ((SPI1->SR & status) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    return 1U;
}

void spi1_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER &= ~(0x3U << (SPI1_SCK * 2U) | 0x3U << (SPI1_MISO * 2U) | 0x3U << (SPI1_MOSI * 2U));
    GPIOA->MODER |= (0x2U << (SPI1_SCK * 2U) | 0x2U << (SPI1_MISO * 2U) | 0x2U << (SPI1_MOSI * 2U));
    GPIOA->OTYPER &= ~((0x1U << SPI1_SCK) | (0x1U << SPI1_MISO) | (0x1U << SPI1_MOSI));
    GPIOA->OSPEEDR |= (0x3U << (SPI1_SCK * 2U) | 0x3U << (SPI1_MISO * 2U) | 0x3U << (SPI1_MOSI * 2U));
    GPIOA->PUPDR &= ~(0x3U << (SPI1_SCK * 2U) | 0x3U << (SPI1_MISO * 2U) | 0x3U << (SPI1_MOSI * 2U));
    
    gpio_output_init(GPIOA, SPI1_CS);
    spi1_cs_high();

    GPIOA->AFR[0] &= ~((0xFU << (SPI1_SCK * 4U)) | (0xFU << (SPI1_MISO * 4U)) | (0xFU << (SPI1_MOSI * 4U)));
    GPIOA->AFR[0] |= (0x5U << (SPI1_SCK * 4U)) | (0x5U << (SPI1_MISO * 4U)) | (0x5U << (SPI1_MOSI * 4U));

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);
    SPI1->CR1 |= SPI_CR1_SSI_Msk;

    SPI1->CFG1 &= ~(SPI_CFG1_DSIZE_Msk | SPI_CFG1_FTHLV_Msk | SPI_CFG1_MBR_Msk);
    SPI1->CFG1 |= (0x7U << SPI_CFG1_DSIZE_Pos) | (0x5U << SPI_CFG1_MBR_Pos);

    SPI1->CFG2 &= ~((SPI_CFG2_COMM_Msk) | (SPI_CFG2_SP_Msk) | (0x1U << SPI_CFG2_LSBFRST_Pos) | (0x1U << SPI_CFG2_CPHA_Pos) | (0x1U << SPI_CFG2_CPOL_Pos));

    SPI1->CFG2 |= SPI_CFG2_SSM_Msk;
    SPI1->IFCR = SPI_IFCR_MODFC_Msk;
    SPI1->CFG2 |= SPI_CFG2_MASTER_Msk;

    current_config.frequency = 750000U;
    current_config.cpol = 0U;
    current_config.cpha = 0U;
    current_config.bit_order = 0U;
    current_config.data_size = 8U;
}

uint8_t spi1_config(const spi_config_t *config)
{
    if (config == NULL) return 0U;
    if (config->frequency == 0U) return 0U;
    if (config->cpol > 1U) return 0U;
    if (config->cpha > 1U) return 0U;
    if (config->bit_order > 1U) return 0U;
    if (config->data_size != 8U) return 0U;

    uint8_t mbr;

    switch(config->frequency)
    {
        case 375000U:
            mbr = 0x6U;
            break;
        case 750000U:
            mbr = 0x5U;
            break;
        case 1500000U:
            mbr = 0x4U;
            break;
        case 3000000U:
            mbr = 0x3U;
            break;
        default:
            return 0U;
    }

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);

    SPI1->CFG1 &= ~SPI_CFG1_MBR_Msk;
    SPI1->CFG1 |= (mbr << SPI_CFG1_MBR_Pos);

    SPI1->CFG2 &= ~((0x1U << SPI_CFG2_CPOL_Pos) | (0x1U << SPI_CFG2_CPHA_Pos));
    SPI1->CFG2 |= (config->cpol << SPI_CFG2_CPOL_Pos) | (config->cpha << SPI_CFG2_CPHA_Pos);

    if (config->bit_order == 0U)
    {
        SPI1->CFG2 &= ~(0x1U << SPI_CFG2_LSBFRST_Pos);
    }
    else
    {
        SPI1->CFG2 |= (0x1U << SPI_CFG2_LSBFRST_Pos);
    }

    SPI1->CFG1 &= ~SPI_CFG1_DSIZE_Msk;
    SPI1->CFG1 |= ((uint32_t)(config->data_size - 1U) << SPI_CFG1_DSIZE_Pos);
    SPI1->CR1 |= SPI_CR1_SPE_Msk;

    current_config = *config;
    return 1U;
}

uint8_t spi1_transfer_byte(uint8_t tx, uint8_t *rx)
{
    if (rx == NULL)
    {
        return 0U;
    }

    if (current_config.data_size != 8U)
    {
        return 0U;
    }

    if ((current_config.cpol > 1U) || (current_config.cpha > 1U) || (current_config.bit_order > 1U))
    {
        return 0U;
    }

    SPI1->IFCR = (SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk | SPI_IFCR_OVRC_Msk);

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);
    SPI1->CR2 &= ~(SPI_CR2_TSIZE_Msk);
    SPI1->CR2 |= (1U << SPI_CR2_TSIZE_Pos);
    SPI1->CR1 |= (0x1U << SPI_CR1_SPE_Pos);

    spi1_cs_low();

    if (spi1_wait_status(SPI_SR_TXP_Msk) == 0U)
    {
        spi1_cs_high();
        return 0U;
    }
    
    *(volatile uint8_t *)&SPI1->TXDR = tx;

    SPI1->CR1 |= (1U << SPI_CR1_CSTART_Pos);

    if (spi1_wait_status(SPI_SR_RXP_Msk) == 0U)
    {
        spi1_cs_high();
        return 0U;
    }
    *rx = *(volatile uint8_t *)&SPI1->RXDR;
    if (spi1_wait_status(SPI_SR_EOT_Msk) == 0U)
    {
        spi1_cs_high();
        return 0U;
    }
    SPI1->IFCR = SPI_IFCR_EOTC_Msk;

    spi1_cs_high();
    return 1U;
}

uint8_t spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t length)
{
    if ((tx == NULL) || (rx == NULL) || (length == 0U))
    {
        return 0U;
    }

    if (current_config.data_size != 8U)
    {
        return 0U;
    }

    if ((current_config.cpol > 1U) || (current_config.cpha > 1U) || (current_config.bit_order > 1U))
    {
        return 0U;
    }

    SPI1->CR1 &= ~(SPI_CR1_SPE_Msk);
    SPI1->CR2 &= ~(SPI_CR2_TSIZE_Msk);
    SPI1->CR2 |= (length << SPI_CR2_TSIZE_Pos);
    SPI1->IFCR = (SPI_IFCR_EOTC_Msk | SPI_IFCR_TXTFC_Msk | SPI_IFCR_OVRC_Msk);
    SPI1->CR1 |= (0x1U << SPI_CR1_SPE_Pos);


    spi1_cs_low();

    if (spi1_wait_status(SPI_SR_TXP_Msk) == 0U)
    {
        spi1_cs_high();
        return 0U;
    }
    *(volatile uint8_t *)&SPI1->TXDR = tx[0];

    SPI1->CR1 |= SPI_CR1_CSTART_Msk;

    for (uint16_t i = 0U; i < length; i++)
    {
        if (i > 0U)
        {
            if (spi1_wait_status(SPI_SR_TXP_Msk) == 0U)
            {
                spi1_cs_high();
                return 0U;
            }
            *(volatile uint8_t *)&SPI1->TXDR = tx[i];
        }

        if (spi1_wait_status(SPI_SR_RXP_Msk) == 0U)
        {
            spi1_cs_high();
            return 0U;
        }
        rx[i] = *(volatile uint8_t *)&SPI1->RXDR;
    }

    if (spi1_wait_status(SPI_SR_EOT_Msk) == 0U)
    {
        spi1_cs_high();
        return 0U;
    }
    SPI1->IFCR = SPI_IFCR_EOTC_Msk;

    spi1_cs_high();
    
    return 1U;
}