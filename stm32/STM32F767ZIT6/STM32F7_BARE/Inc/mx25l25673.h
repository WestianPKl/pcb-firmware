#ifndef MX25L25673_H
#define MX25L25673_H

#include <stdint.h>
#include "stm32f7xx.h"

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

#define MX25_ID_MANUFACTURER  0xC2u
#define MX25_ID_MEMORY_TYPE   0x20u
#define MX25_ID_CAPACITY      0x19u

#define MX25_RESET_RECOVERY_MS 40u
#define MX25_READY_TIMEOUT_MS  100u

int mx25_init(void);
int mx25_read_id(uint8_t *id);
int mx25_read_status(uint8_t *status);
int mx25_wait_ready(uint32_t timeout_ms);

void mx25_hold_high(void);
void mx25_hold_low(void);

// int mx25_read(uint32_t addr, uint8_t *dst, uint32_t len);
// int mx25_write(uint32_t addr, const uint8_t *src, uint32_t len);
// int mx25_sector_erase_4k(uint32_t addr);


#endif // MX25L25673_H