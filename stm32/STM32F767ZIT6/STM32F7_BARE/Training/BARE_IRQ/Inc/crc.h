#ifndef CRC_H
#define CRC_H

#include <stdint.h>
#include "stm32f7xx.h"

uint32_t crc32_words(uint32_t *data, uint32_t length);
uint32_t crc32_bytes(const uint8_t *data, uint32_t len);

#endif // CRC_H