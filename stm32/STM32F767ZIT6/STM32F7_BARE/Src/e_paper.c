#include "e_paper.h"
#include "spi.h"
#include "main.h"
#include <stdio.h>

static int busy_wait_high(uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount();
    while ((GPIOD->IDR & (1U << BUSY_PIN)) == 0) {
        vTaskDelay(pdMS_TO_TICKS(5));
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms)) {
            printf("E-PAPER: BUSY timeout, IDR=0x%04lX\n", (unsigned long)GPIOD->IDR);
            return 0;
        }
    }
    return 1;
}

static void e_paper_rst_low(void)
{
    GPIOD->BSRR = (1U << (RST_PIN + 16));
}

static void e_paper_rst_high(void)
{
    GPIOD->BSRR = (1U << RST_PIN);
}

static void e_paper_dc_low(void)
{
    GPIOD->BSRR = (1U << (DC_PIN + 16));
}

static void e_paper_dc_high(void)
{
    GPIOD->BSRR = (1U << DC_PIN);
}

static void e_paper_reset(void)
{
    e_paper_rst_high();
    vTaskDelay(pdMS_TO_TICKS(20));
    e_paper_rst_low();
    vTaskDelay(pdMS_TO_TICKS(1));
    e_paper_rst_high();
    vTaskDelay(pdMS_TO_TICKS(20));
}

static void e_paper_send_command(uint8_t cmd)
{
    e_paper_dc_low();
    spi1_cs_low();
    spi1_xfer(cmd);
    while (SPI1->SR & SPI_SR_BSY) { }
    spi1_cs_high();
}

static void e_paper_send_data(uint8_t data)
{
    e_paper_dc_high();
    spi1_cs_low();
    spi1_xfer(data);
    while (SPI1->SR & SPI_SR_BSY) { }
    spi1_cs_high();
}

static void e_paper_turn_on_display(void)
{
    e_paper_send_command(0x12);
    e_paper_send_data(0x01);

    if (!busy_wait_high(35000)) return;

    e_paper_send_command(0x02);
    e_paper_send_data(0x00);
    busy_wait_high(5000);
}

void e_paper_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~(0x3U << (BUSY_PIN * 2U));
    GPIOD->MODER |= (0x0U << (BUSY_PIN * 2U));
    /* pull-down: a missing/dead module reads 0 (busy) instead of being
       masked as "ready" by a pull-up; a live controller drives the pin high */
    GPIOD->PUPDR &= ~(0x3U << (BUSY_PIN * 2U));
    GPIOD->PUPDR |= (0x2U << (BUSY_PIN * 2U));

    GPIOD->MODER &= ~(0x3U << (DC_PIN * 2U));
    GPIOD->MODER &= ~(0x3U << (RST_PIN * 2U));
    GPIOD->MODER |=  (0x1U << (DC_PIN * 2U));
    GPIOD->MODER |=  (0x1U << (RST_PIN * 2U));

    GPIOD->PUPDR &= ~(0x3U << (DC_PIN * 2U));
    GPIOD->PUPDR &= ~(0x3U << (RST_PIN * 2U));

    if (xSemaphoreTake(spi_mutex, portMAX_DELAY) != pdTRUE) return;

    printf("EPD: BUSY before reset=%lu\n", (unsigned long)((GPIOD->IDR >> BUSY_PIN) & 1U));
    e_paper_reset();
    printf("EPD: BUSY after reset=%lu\n", (unsigned long)((GPIOD->IDR >> BUSY_PIN) & 1U));

    e_paper_send_command(0x66);
    e_paper_send_data(0x49);
    e_paper_send_data(0x55);
    e_paper_send_data(0x13);
    e_paper_send_data(0x5D);

    e_paper_send_command(0x66);
    e_paper_send_data(0x49);
    e_paper_send_data(0x55);

    e_paper_send_command(0xB0);
    e_paper_send_data(0x03);

    e_paper_send_command(0x00);
    e_paper_send_data(0x4F);
    e_paper_send_data(0x6B);

    e_paper_send_command(0x03);
    e_paper_send_data(0x00);

    e_paper_send_command(0xF0);
    e_paper_send_data(0xF6);
    e_paper_send_data(0x0D);
    e_paper_send_data(0x00);
    e_paper_send_data(0x00);
    e_paper_send_data(0x00);

    e_paper_send_command(0x06);
    e_paper_send_data(0xCF);
    e_paper_send_data(0xDF);
    e_paper_send_data(0x0F);

    e_paper_send_command(0x41);
    e_paper_send_data(0x00);

    e_paper_send_command(0x50);
    e_paper_send_data(0x30);

    e_paper_send_command(0x60);
    e_paper_send_data(0x0C);
    e_paper_send_data(0x05);

    e_paper_send_command(0x61);
    e_paper_send_data(0xA8);
    e_paper_send_data(0x00);
    e_paper_send_data(0xA8);

    e_paper_send_command(0x84);
    e_paper_send_data(0x01);

    xSemaphoreGive(spi_mutex);
}

void e_paper_clear(uint8_t color)
{
    uint16_t width, height;
    width = (EPD_1IN64G_WIDTH % 4 == 0)? (EPD_1IN64G_WIDTH / 4 ): (EPD_1IN64G_WIDTH / 4 + 1);
    height = EPD_1IN64G_HEIGHT;
    uint8_t byte = (uint8_t)((color << 6) | (color << 4) | (color << 2) | color);

    if (xSemaphoreTake(spi_mutex, portMAX_DELAY) != pdTRUE) return;

    e_paper_send_command(0x68);
    e_paper_send_data(0x01);

    e_paper_send_command(0x04);
    busy_wait_high(5000);

    e_paper_send_command(0x10);
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            e_paper_send_data(byte);
        }
    }

    e_paper_send_command(0x68);
    e_paper_send_data(0x00);

    e_paper_turn_on_display();

    xSemaphoreGive(spi_mutex);
}

void e_paper_display(const uint8_t *image)
{
    uint16_t width, height;
    width = (EPD_1IN64G_WIDTH % 4 == 0)? (EPD_1IN64G_WIDTH / 4 ): (EPD_1IN64G_WIDTH / 4 + 1);
    height = EPD_1IN64G_HEIGHT;

    if (xSemaphoreTake(spi_mutex, portMAX_DELAY) != pdTRUE) return;

    e_paper_send_command(0x68);
    e_paper_send_data(0x01);

    e_paper_send_command(0x04);
    busy_wait_high(5000);

    e_paper_send_command(0x10);
    for (uint16_t j = 0; j < height; j++) {
        for (uint16_t i = 0; i < width; i++) {
            e_paper_send_data(image[i + j * width]);
        }
    }

    e_paper_send_command(0x68);
    e_paper_send_data(0x00);

    e_paper_turn_on_display();

    xSemaphoreGive(spi_mutex);
}


void e_paper_sleep(void)
{
    if (xSemaphoreTake(spi_mutex, portMAX_DELAY) != pdTRUE) return;

    e_paper_send_command(0x02);
    e_paper_send_data(0x00);
    e_paper_send_command(0x07);
    e_paper_send_data(0xA5);

    xSemaphoreGive(spi_mutex);
}