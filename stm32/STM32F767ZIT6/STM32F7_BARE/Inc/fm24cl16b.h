#ifndef FM24CL16B_H
#define FM24CL16B_H

#include <stdint.h>
#include "stm32f7xx.h"

#define FM24CL16B_I2C_ADDR_BASE   0x50
#define FM24CL16B_SIZE_BYTES      2048u
#define FM24CL16B_ADDR_MASK       0x07FFu

#define FRAM_ADDR_FLAGS                 0x005

#define FRAM_ADDR_DEVICE_ID             0x008
#define FRAM_ADDR_SENSOR_ID             0x028
#define FRAM_ADDR_WRITE_INDEX           0x048
#define FRAM_ADDR_COUNT                 0x068
#define FRAM_ADDR_NEXT_SEQUENCE         0x088
#define FRAM_ADDR_MEASURE_INTERVAL      0x128
#define FRAM_ADDR_LOG_META_A            0x180u
#define FRAM_ADDR_LOG_META_B            0x1C0u

#define FM24CL16B_DEV_ADDR(a) \
    (uint8_t)(FM24CL16B_I2C_ADDR_BASE | (((a) >> 8) & 0x07))

#define FM24CL16B_WORD_ADDR(a) \
    (uint8_t)((a) & 0xFF)

int fm24cl16b_write(uint16_t mem_addr, const uint8_t *src, uint16_t len);
int fm24cl16b_read(uint16_t mem_addr, uint8_t *dst, uint16_t len);

int fm24cl16b_write_byte(uint16_t mem_addr, uint8_t value);
int fm24cl16b_read_byte(uint16_t mem_addr, uint8_t *value);

int fm24cl16b_write_u32(uint16_t mem_addr, uint32_t value);
int fm24cl16b_read_u32(uint16_t mem_addr, uint32_t *value);

#endif // FM24CL16B_H