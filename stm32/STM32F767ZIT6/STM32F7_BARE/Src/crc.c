#include "crc.h"

uint32_t crc32_words(uint32_t *data, uint32_t length)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR;

    CRC->CR = CRC_CR_RESET;

    for (uint32_t i = 0; i < length; i++) {
        CRC->DR = data[i];
    }

    return CRC->DR;
}

uint32_t crc32_bytes(const uint8_t *data, uint32_t len)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_CRCEN;
    (void)RCC->AHB1ENR;

    CRC->CR = CRC_CR_RESET;

    for (uint32_t i = 0; i < len; i++) {
        *(volatile uint8_t *)&CRC->DR = data[i];
    }
    return CRC->DR;
}