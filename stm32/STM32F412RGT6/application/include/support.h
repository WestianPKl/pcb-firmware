#ifndef SUPPORT_H
#define SUPPORT_H

#include <stdint.h>

uint8_t crc8_atm(const uint8_t *data, uint32_t len);
uint16_t crc16_ccitt_false(const uint8_t *data, uint16_t len);
uint32_t crc32(const uint8_t *data, uint32_t len);

#endif // SUPPORT_H
