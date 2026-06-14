#ifndef MX25L25673GM2I_H
#define MX25L25673GM2I_H

#include "stm32f4xx.h"
#include <stdint.h>

#define MX25_PAGE_SIZE        256u
#define MX25_SECTOR_SIZE      4096u
#define MX25_SIZE_BYTES       33554432u
#define MX25_MAX_ADDR         0x01FFFFFFu

#define MX25_CMD_WREN         0x06u
#define MX25_CMD_RDSR         0x05u
#define MX25_CMD_READ         0x03u
#define MX25_CMD_PP           0x02u
#define MX25_CMD_SE           0x20u
#define MX25_CMD_RDID         0x9Fu
#define MX25_CMD_EN4B         0xB7u
#define MX25_CMD_RSTEN        0x66u
#define MX25_CMD_RST          0x99u

#define MX25_SR_WIP           0x01u
#define MX25_SR_WEL           0x02u

typedef struct __attribute__((packed)) {
    int32_t  temp_x100;     // np. 2534 = 25.34 C
    uint32_t hum_x100;      // np. 4567 = 45.67 %
    uint32_t press_pa;      // 0 dla SHT40
    uint8_t  year;          // np. 25 = 2025 albo 26 = 2026
    uint8_t  month;
    uint8_t  day;
    uint8_t  hour;
    uint8_t  min;
    uint8_t  sec;
} mx25_record_t;

int mx25_init(void);

int mx25_read(uint32_t addr, uint8_t *dst, uint32_t len);
int mx25_write(uint32_t addr, const uint8_t *src, uint32_t len);
int mx25_sector_erase_4k(uint32_t addr);

#endif