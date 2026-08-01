/*-----------------------------------------------------------------------*/
/* Low level disk I/O for FatFs — SD card over SPI1 (CS: PF12)          */
/*-----------------------------------------------------------------------*/

#include "ff.h"
#include "diskio.h"
#include "spi.h"
#include "FreeRTOS.h"
#include "task.h"
#include "rtc.h"
#include "main.h"

#define CMD0    (0)
#define CMD8    (8)
#define CMD9    (9)
#define CMD12   (12)
#define CMD16   (16)
#define CMD17   (17)
#define CMD18   (18)
#define CMD24   (24)
#define CMD25   (25)
#define CMD55   (55)
#define CMD58   (58)
#define ACMD23  (0x80 | 23)
#define ACMD41  (0x80 | 41)

#define CT_MMC    0x01
#define CT_SD1    0x02
#define CT_SD2    0x04
#define CT_BLOCK  0x08

#define SD_BR_INIT 7U /* /256 ~= 422 kHz */
#define SD_BR_FAST 2U /* /8 = 13.5 MHz, SD default speed limit is 25 MHz */

static uint8_t card_type = 0;
static DSTATUS disk_stat = STA_NOINIT;

/* Takes the shared SPI1 bus, remembers the current prescaler and sets the
   SD one; sd_bus_release() restores it so other SPI1 devices keep their speed. */
static int sd_bus_acquire(uint8_t br, uint8_t *saved_br)
{
    if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(1000)) != pdTRUE) return 0;
    *saved_br = (uint8_t)((SPI1->CR1 & SPI_CR1_BR_Msk) >> SPI_CR1_BR_Pos);
    spi1_set_prescaler(br);
    return 1;
}

static void sd_bus_release(uint8_t saved_br)
{
    spi1_set_prescaler(saved_br);
    xSemaphoreGive(spi_mutex);
}

DWORD get_fattime(void)
{
    rtc_datetime_t dt = {
        .seconds = 0, .minutes = 0, .hours = 0,
        .weekday = 4, .day = 1, .month = 1, .year = 2026
    };

    if (xSemaphoreTake(rtc_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        rtc_get_datetime(&dt);
        xSemaphoreGive(rtc_mutex);
    }

    return ((DWORD)(dt.year  - 1980) << 25) |
           ((DWORD) dt.month         << 21) |
           ((DWORD) dt.day           << 16) |
           ((DWORD) dt.hours         << 11) |
           ((DWORD) dt.minutes       <<  5) |
           ((DWORD) dt.seconds       >>  1);
}

static int sd_wait_ready(uint32_t timeout_ms)
{
    uint32_t start = xTaskGetTickCount();
    while (spi1_xfer(0xFF) != 0xFF) {
        if ((xTaskGetTickCount() - start) >= pdMS_TO_TICKS(timeout_ms))
            return 0;
    }
    return 1;
}

static void sd_deselect(void)
{
    spi1_cs_high();
    spi1_xfer(0xFF);
}

static int sd_select(void)
{
    spi1_cs_low();
    spi1_xfer(0xFF);
    if (sd_wait_ready(500)) return 1;
    sd_deselect();
    return 0;
}

static uint8_t sd_send_cmd(uint8_t cmd, uint32_t arg)
{
    uint8_t crc, res, n;

    if (cmd & 0x80) {
        cmd &= 0x7F;
        res = sd_send_cmd(CMD55, 0);
        if (res > 1) return res;
    }

    if (cmd != CMD12) {
        sd_deselect();
        if (!sd_select()) return 0xFF;
    }

    spi1_xfer(0x40 | cmd);
    spi1_xfer((uint8_t)(arg >> 24));
    spi1_xfer((uint8_t)(arg >> 16));
    spi1_xfer((uint8_t)(arg >>  8));
    spi1_xfer((uint8_t)(arg));

    crc = 0x01;
    if (cmd == CMD0) crc = 0x95;
    if (cmd == CMD8) crc = 0x87;
    spi1_xfer(crc);

    if (cmd == CMD12) spi1_xfer(0xFF);

    n = 10;
    do { res = spi1_xfer(0xFF); } while ((res & 0x80) && --n);
    return res;
}

static int sd_recv_block(BYTE *buf, UINT len)
{
    uint8_t token;
    uint32_t start = xTaskGetTickCount();
    do {
        token = spi1_xfer(0xFF);
    } while ((token == 0xFF) && (xTaskGetTickCount() - start) < pdMS_TO_TICKS(200));

    if (token != 0xFE) return 0;
    for (UINT i = 0; i < len; i++) buf[i] = spi1_xfer(0xFF);
    spi1_xfer(0xFF); spi1_xfer(0xFF);
    return 1;
}

#if FF_FS_READONLY == 0
static int sd_send_block(const BYTE *buf, uint8_t token)
{
    if (!sd_wait_ready(500)) return 0;
    spi1_xfer(token);
    if (token != 0xFD) {
        for (UINT i = 0; i < 512; i++) spi1_xfer(buf[i]);
        spi1_xfer(0xFF); spi1_xfer(0xFF);
        if ((spi1_xfer(0xFF) & 0x1F) != 0x05) return 0;
    }
    return 1;
}
#endif

DSTATUS disk_status(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    return disk_stat;
}

DSTATUS disk_initialize(BYTE pdrv)
{
    if (pdrv != 0) return STA_NOINIT;
    uint8_t n, ty, buf[4], saved_br;

    if (!sd_bus_acquire(SD_BR_INIT, &saved_br)) return STA_NOINIT;

    vTaskDelay(pdMS_TO_TICKS(10)); /* card VDD settle time before init clocks */
    spi1_cs_high();
    for (n = 10; n; n--) spi1_xfer(0xFF);

    ty = 0;
    if (sd_send_cmd(CMD0, 0) == 1) {
        if (sd_send_cmd(CMD8, 0x1AA) == 1) {
            for (n = 0; n < 4; n++) buf[n] = spi1_xfer(0xFF);
            if (buf[2] == 0x01 && buf[3] == 0xAA) {
                uint32_t start = xTaskGetTickCount();
                uint8_t racmd;
                do {
                    /* HCS bit set - SDHC/SDXC cards never leave idle without it */
                    racmd = sd_send_cmd(ACMD41, 1UL << 30);
                    if (racmd) vTaskDelay(pdMS_TO_TICKS(1));
                } while (racmd &&
                              (xTaskGetTickCount() - start) < pdMS_TO_TICKS(1000));
                if (racmd == 0 && sd_send_cmd(CMD58, 0) == 0) {
                    for (n = 0; n < 4; n++) buf[n] = spi1_xfer(0xFF);
                    ty = (buf[0] & 0x40) ? (CT_SD2 | CT_BLOCK) : CT_SD2;
                }
            }
        } else {
            uint8_t cmd, r;
            if (sd_send_cmd(ACMD41, 0) <= 1) { ty = CT_SD1; cmd = ACMD41; }
            else                              { ty = CT_MMC; cmd = 1;      }
            uint32_t start = xTaskGetTickCount();
            do {
                r = sd_send_cmd(cmd, 0);
                if (r) vTaskDelay(pdMS_TO_TICKS(1));
            } while (r && (xTaskGetTickCount() - start) < pdMS_TO_TICKS(1000));
            if (r || sd_send_cmd(CMD16, 512) != 0) ty = 0;
        }
    }

    card_type = ty;
    sd_deselect();
    sd_bus_release(saved_br);
    disk_stat = ty ? 0 : STA_NOINIT;
    return disk_stat;
}

DRESULT disk_read(BYTE pdrv, BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || (disk_stat & STA_NOINIT)) return RES_NOTRDY;
    uint8_t saved_br;
    if (!sd_bus_acquire(SD_BR_FAST, &saved_br)) return RES_NOTRDY;
    if (!(card_type & CT_BLOCK)) sector *= 512;

    if (count == 1) {
        if (sd_send_cmd(CMD17, sector) == 0 && sd_recv_block(buff, 512)) count = 0;
    } else {
        if (sd_send_cmd(CMD18, sector) == 0) {
            do { if (!sd_recv_block(buff, 512)) break; buff += 512; } while (--count);
            sd_send_cmd(CMD12, 0);
        }
    }
    sd_deselect();
    sd_bus_release(saved_br);
    return count ? RES_ERROR : RES_OK;
}

#if FF_FS_READONLY == 0
DRESULT disk_write(BYTE pdrv, const BYTE *buff, LBA_t sector, UINT count)
{
    if (pdrv != 0 || (disk_stat & STA_NOINIT)) return RES_NOTRDY;
    uint8_t saved_br;
    if (!sd_bus_acquire(SD_BR_FAST, &saved_br)) return RES_NOTRDY;
    if (!(card_type & CT_BLOCK)) sector *= 512;

    if (count == 1) {
        if (sd_send_cmd(CMD24, sector) == 0 && sd_send_block(buff, 0xFE)) count = 0;
    } else {
        if (card_type & CT_SD1) sd_send_cmd(ACMD23, count);
        if (sd_send_cmd(CMD25, sector) == 0) {
            do { if (!sd_send_block(buff, 0xFC)) break; buff += 512; } while (--count);
            if (!sd_send_block(NULL, 0xFD)) count = 1;
        }
    }
    sd_deselect();
    sd_bus_release(saved_br);
    return count ? RES_ERROR : RES_OK;
}
#endif

DRESULT disk_ioctl(BYTE pdrv, BYTE cmd, void *buff)
{
    if (pdrv != 0 || (disk_stat & STA_NOINIT)) return RES_NOTRDY;
    uint8_t saved_br;
    if (!sd_bus_acquire(SD_BR_FAST, &saved_br)) return RES_NOTRDY;
    DRESULT res = RES_ERROR;

    switch (cmd) {
    case CTRL_SYNC:
        if (sd_select()) res = RES_OK;
        break;
    case GET_SECTOR_SIZE:
        *(WORD *)buff = 512; res = RES_OK; break;
    case GET_BLOCK_SIZE:
        *(DWORD *)buff = 128; res = RES_OK; break;
    case GET_SECTOR_COUNT: {
        uint8_t csd[16];
        if (sd_send_cmd(CMD9, 0) == 0 && sd_recv_block(csd, 16)) {
            if ((csd[0] >> 6) == 1) {
                uint32_t csize = ((uint32_t)(csd[8] & 0x3F) << 16) | ((uint32_t)csd[9] << 8) | csd[10];
                *(DWORD *)buff = (csize + 1) << 10;
            } else {
                uint32_t n = (csd[5] & 0x0F) + ((csd[10] & 0x80) >> 7) + ((csd[9] & 0x03) << 1) + 2;
                uint32_t csize = ((uint32_t)(csd[6] & 0x03) << 10) | ((uint32_t)csd[7] << 2) | (csd[8] >> 6);
                *(DWORD *)buff = (csize + 1) << (n - 9);
            }
            res = RES_OK;
        }
        break;
    }
    default: res = RES_PARERR; break;
    }

    sd_deselect();
    sd_bus_release(saved_br);
    return res;
}