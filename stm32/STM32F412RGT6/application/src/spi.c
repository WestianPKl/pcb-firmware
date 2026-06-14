#include "spi.h"

#define SPI1_CS_PORT   GPIOA
#define SPI1_CS_PIN    4U

// SPI1: PA4 = CS_BME, PA5 = SCK, PA6 = MISO, PA7 = MOSI

#define SPI1_SCK_PIN    5U
#define SPI1_MISO_PIN   6U
#define SPI1_MOSI_PIN   7U

void spi1_cs_high(void)
{
    SPI1_CS_PORT->BSRR = (1U << SPI1_CS_PIN);
}

void spi1_cs_low(void)
{
    SPI1_CS_PORT->BSRR = (1U << (SPI1_CS_PIN + 16U));
}

void spi1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~(3U << (SPI1_CS_PIN * 2U));
    GPIOA->MODER |=  (1U << (SPI1_CS_PIN * 2U));
    GPIOA->OTYPER &= ~(1U << SPI1_CS_PIN);
    GPIOA->OSPEEDR |= (3U << (SPI1_CS_PIN * 2U));
    GPIOA->PUPDR &= ~(3U << (SPI1_CS_PIN * 2U));
    spi1_cs_high();

    GPIOA->MODER &= ~((3U << (SPI1_SCK_PIN * 2U)) | (3U << (SPI1_MISO_PIN * 2U)) | (3U << (SPI1_MOSI_PIN * 2U)));
    GPIOA->MODER |=  ((2U << (SPI1_SCK_PIN * 2U)) | (2U << (SPI1_MISO_PIN * 2U)) | (2U << (SPI1_MOSI_PIN * 2U)));

    GPIOA->OTYPER &= ~((1U << SPI1_SCK_PIN) | (1U << SPI1_MISO_PIN) | (1U << SPI1_MOSI_PIN));
    GPIOA->OSPEEDR |= ((3U << (SPI1_SCK_PIN * 2U)) | (3U << (SPI1_MISO_PIN * 2U)) | (3U << (SPI1_MOSI_PIN * 2U)));
    GPIOA->PUPDR &= ~((3U << (SPI1_SCK_PIN * 2U)) | (3U << (SPI1_MISO_PIN * 2U)) | (3U << (SPI1_MOSI_PIN * 2U)));

    GPIOA->AFR[0] &= ~((0xFU << (SPI1_SCK_PIN * 4U)) | (0xFU << (SPI1_MISO_PIN * 4U)) | (0xFU << (SPI1_MOSI_PIN * 4U)));
    GPIOA->AFR[0] |=  ((5U   << (SPI1_SCK_PIN * 4U)) | (5U   << (SPI1_MISO_PIN * 4U)) | (5U   << (SPI1_MOSI_PIN * 4U)));

    RCC->APB2ENR |= RCC_APB2ENR_SPI1EN;
    (void)RCC->APB2ENR;

    SPI1->I2SCFGR = 0;

    SPI1->CR1 = 0;
    SPI1->CR2 = 0;

    SPI1->CR1 |= SPI_CR1_MSTR;
    SPI1->CR1 |= (3U << SPI_CR1_BR_Pos);
    SPI1->CR1 |= SPI_CR1_CPOL | SPI_CR1_CPHA;

    SPI1->CR1 &= ~SPI_CR1_LSBFIRST;
    SPI1->CR1 &= ~SPI_CR1_RXONLY;

    SPI1->CR1 &= ~SPI_CR1_DFF;
    SPI1->CR1 |= SPI_CR1_SSM | SPI_CR1_SSI;
    SPI1->CR2 |= SPI_CR2_TXDMAEN | SPI_CR2_RXDMAEN;

    SPI1->CR1 |= SPI_CR1_SPE;
}