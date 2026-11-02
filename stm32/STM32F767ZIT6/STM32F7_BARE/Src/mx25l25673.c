#include "mx25l25673.h"
#include "spi.h"
#include "support.h"
#include <stdio.h>

#define MX25_CS 7U // PF7
#define MX25_HOLD 9U // PF9

static void mx25_cs_high(void)
{
    GPIOF->BSRR = (1U << MX25_CS);
}

static void mx25_cs_low(void)
{
    GPIOF->BSRR = (1U << (MX25_CS + 16));
}

void mx25_hold_high(void)
{
    GPIOF->BSRR = (1U << MX25_HOLD);
}

void mx25_hold_low(void)
{
    GPIOF->BSRR = (1U << (MX25_HOLD + 16));
}

static void mx25_cs_release(void)
{
    while (SPI1->SR & SPI_SR_BSY) { }
    mx25_cs_high();
}

static void mx25_gpio_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    (void)RCC->AHB1ENR;

    GPIOF->BSRR = (1U << MX25_CS) | (1U << MX25_HOLD);

    GPIOF->MODER &= ~((3U << (MX25_CS * 2)) | (3U << (MX25_HOLD * 2)));
    GPIOF->MODER |= (1U << (MX25_CS * 2)) | (1U << (MX25_HOLD * 2));
    GPIOF->OTYPER &= ~((1U << MX25_CS) | (1U << MX25_HOLD));
    GPIOF->OSPEEDR |= (3U << (MX25_CS * 2)) | (3U << (MX25_HOLD * 2));
    GPIOF->PUPDR &= ~((3U << (MX25_CS * 2)) | (3U << (MX25_HOLD * 2)));
}

static void mx25_cmd(uint8_t cmd)
{
    mx25_cs_low();
    spi1_transfer(&cmd, NULL, 1U);
    mx25_cs_release();
}

int mx25_read_id(uint8_t *id)
{
    uint8_t txbuf[4] = {MX25_CMD_RDID, 0xFFU, 0xFFU, 0xFFU};
    uint8_t rxbuf[4] = {0};

    if (!id) return 0;

    mx25_cs_low();
    spi1_transfer(txbuf, rxbuf, 4U);
    mx25_cs_release();

    id[0] = rxbuf[1];
    id[1] = rxbuf[2];
    id[2] = rxbuf[3];

    return 1;
}

int mx25_read_status(uint8_t *status)
{
    uint8_t txbuf[2] = {MX25_CMD_RDSR, 0xFFU};
    uint8_t rxbuf[2] = {0};

    if (!status) return 0;

    mx25_cs_low();
    spi1_transfer(txbuf, rxbuf, 2U);
    mx25_cs_release();

    *status = rxbuf[1];

    return 1;
}

int mx25_wait_ready(uint32_t timeout_ms)
{
    uint8_t status = 0;

    for (uint32_t i = 0; i <= timeout_ms; i++) {
        if (!mx25_read_status(&status)) return 0;
        if (!(status & MX25_SR_WIP)) return 1;
        delay_ms(1U);
    }

    return 0;
}

int mx25_init(void)
{
    uint8_t id[3] = {0};

    mx25_gpio_init();

    mx25_cmd(MX25_CMD_RSTEN);
    mx25_cmd(MX25_CMD_RST);
    delay_ms(MX25_RESET_RECOVERY_MS);

    if (!mx25_wait_ready(MX25_READY_TIMEOUT_MS)) {
        printf("MX25L25673: busy after reset\n");
        return 0;
    }

    mx25_cmd(MX25_CMD_EN4B);

    if (!mx25_read_id(id)) return 0;

    printf("MX25L25673 ID: %02X %02X %02X\n", id[0], id[1], id[2]);

    if ((id[0] != MX25_ID_MANUFACTURER) ||
        (id[1] != MX25_ID_MEMORY_TYPE) ||
        (id[2] != MX25_ID_CAPACITY)) {
        return 0;
    }

    return 1;
}
