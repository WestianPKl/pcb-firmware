#include "spi.h"
#include "support.h"

void spi1_cs_high(void)
{
    GPIOF->BSRR = (1U << SPI1_CS);
}

void spi1_cs_low(void)
{
    GPIOF->BSRR = (1U << (SPI1_CS + 16));
}

void spi3_cs_high(void)
{
    GPIOB->BSRR = (1U << SPI3_CS);
}

void spi3_cs_low(void)
{
    GPIOB->BSRR = (1U << (SPI3_CS + 16));
}

uint8_t spi1_xfer(uint8_t out)
{
    while (!(SPI1->SR & SPI_SR_TXE)) { }
    *(volatile uint8_t *)&SPI1->DR = out;
    while (!(SPI1->SR & SPI_SR_RXNE)) { }
    return *(volatile uint8_t *)&SPI1->DR;
}

void spi1_transfer(const uint8_t *tx, uint8_t *rx, uint16_t len)
{
    for (uint16_t i = 0; i < len; i++) {
        uint8_t in = spi1_xfer(tx ? tx[i] : 0xFFU);
        if (rx) rx[i] = in;
    }
}

void spi1_write_reg(uint8_t reg, uint8_t val)
{
    spi1_cs_low();
    spi1_xfer(reg);
    spi1_xfer(val);
    while (SPI1->SR & SPI_SR_BSY) { }
    spi1_cs_high();
}

uint8_t spi1_read_reg(uint8_t reg)
{
    spi1_cs_low();
    spi1_xfer(reg);
    uint8_t val = spi1_xfer(0xFFU);
    while (SPI1->SR & SPI_SR_BSY) { }
    spi1_cs_high();
    return val;
}

void spi1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

     RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~((3U << (SPI1_SCK * 2)) | (3U << (SPI1_MISO * 2)));
    GPIOA->MODER |= ((2U << (SPI1_SCK * 2)) | (2U << (SPI1_MISO * 2)));
    GPIOA->OTYPER &= ~((1U << SPI1_SCK) | (1U << SPI1_MISO));
    GPIOA->OSPEEDR |= ((3U << (SPI1_SCK * 2)) | (3U << (SPI1_MISO * 2)));
    GPIOA->PUPDR &= ~((3U << (SPI1_SCK * 2)) | (3U << (SPI1_MISO * 2)));
    GPIOA->PUPDR |= (1U << (SPI1_MISO * 2));

    GPIOD->MODER &= ~(3U << (SPI1_MOSI * 2));
    GPIOD->MODER |=  (2U << (SPI1_MOSI * 2));
    GPIOD->OTYPER &= ~(1U << SPI1_MOSI);
    GPIOD->OSPEEDR |= (3U << (SPI1_MOSI * 2));
    GPIOD->PUPDR &= ~(3U << (SPI1_MOSI * 2));

    GPIOA->AFR[0] &= ~((0xFU << (SPI1_SCK * 4)) | (0xFU << (SPI1_MISO * 4)) );
    GPIOA->AFR[0] |= ((5U << (SPI1_SCK * 4)) | (5U << (SPI1_MISO * 4)));
    GPIOD->AFR[0] &= ~(0xFU << (SPI1_MOSI * 4));
    GPIOD->AFR[0] |=  (5U << (SPI1_MOSI * 4));

    GPIOF->MODER &= ~(3U << (SPI1_CS * 2));
    GPIOF->MODER |=  (1U << (SPI1_CS * 2));
    GPIOF->OTYPER &= ~(1U << SPI1_CS);
    GPIOF->OSPEEDR |= (3U << (SPI1_CS * 2));
    GPIOF->PUPDR &= ~(3U << (SPI1_CS * 2));
    GPIOF->BSRR = (1U << SPI1_CS);

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    SPI1->CR1 |= SPI_CR1_MSTR | SPI_CR1_SSM | SPI_CR1_SSI | SPI_CR1_BR_1 | SPI_CR1_BR_2;

    SPI1->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN | SPI_CR2_FRXTH;
    
    SPI1->CR1 &= ~(SPI_CR1_CPOL | SPI_CR1_CPHA);
    SPI1->CR1 &= ~SPI_CR1_RXONLY;

    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi1_set_prescaler(uint8_t br)
{
    SPI1->CR1 &= ~SPI_CR1_SPE;
    SPI1->CR1  = (SPI1->CR1 & ~SPI_CR1_BR_Msk) | ((br & 0x7U) << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_SPE;
}

void spi3_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~((3U << (SPI3_SCK * 2)) | (3U << (SPI3_MISO * 2)) | (3U << (SPI3_MOSI * 2)) | (3U << (SPI3_CS * 2)));
    GPIOB->MODER |= ((2U << (SPI3_SCK * 2)) | (2U << (SPI3_MISO * 2)) | (2U << (SPI3_MOSI * 2)) | (1U << (SPI3_CS * 2)));
    GPIOB->OTYPER &= ~((1U << SPI3_SCK) | (1U << SPI3_MISO) | (1U << SPI3_MOSI) | (1U << SPI3_CS));
    GPIOB->OSPEEDR |= ((3U << (SPI3_SCK * 2)) | (3U << (SPI3_MISO * 2)) | (3U << (SPI3_MOSI * 2)) | (3U << (SPI3_CS * 2)));
    GPIOB->PUPDR &= ~((3U << (SPI3_SCK * 2)) | (3U << (SPI3_MISO * 2)) | (3U << (SPI3_MOSI * 2)) | (3U << (SPI3_CS * 2)));

    GPIOB->AFR[0] &= ~((0xFU << (SPI3_SCK * 4)) | (0xFU << (SPI3_MISO * 4)) | (0xFU << (SPI3_MOSI * 4)) | (0xFU << (SPI3_CS * 4)));
    GPIOB->AFR[0] |= ((6U << (SPI3_SCK * 4)) | (6U << (SPI3_MISO * 4)) | (6U << (SPI3_MOSI * 4)) | (0U << (SPI3_CS * 4)));

    RCC->APB1ENR |= RCC_APB1ENR_SPI3EN;
    (void)RCC->APB1ENR;

    SPI3->CR1 = 0;
    SPI3->CR2 = 0;

    SPI3->CR1 &= ~SPI_CR1_MSTR;
    SPI3->CR1 |=  SPI_CR1_SSM | SPI_CR1_BR_0 |  SPI_CR1_BR_2;

    SPI3->CR2 |= SPI_CR2_RXDMAEN | SPI_CR2_TXDMAEN | SPI_CR2_FRXTH;
    SPI3->CR1 |= SPI_CR1_CPOL | SPI_CR1_CPHA;
    SPI3->CR1 &= ~SPI_CR1_RXONLY;

    SPI3->CR1 |= SPI_CR1_SPE;
}