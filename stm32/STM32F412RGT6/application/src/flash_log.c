#include "flash_log.h"
#include "mx25l25673gm2i.h"
#include "fm24cl16b.h"
#include "support.h"
#include <string.h>

#define FLASH_LOG_BASE_ADDR    0x00000000u
#define FLASH_LOG_AREA_SIZE    MX25_SIZE_BYTES
#define FLASH_LOG_RECORD_SIZE       ((uint32_t)sizeof(flash_log_record_t))
#define FLASH_LOG_CAPACITY          (FLASH_LOG_AREA_SIZE / FLASH_LOG_RECORD_SIZE)
#define FLASH_LOG_META_MAGIC        0x464C4F47u
#define FLASH_LOG_META_VERSION      1u
#define FRAM_ADDR_LOG_META_A        0x180u
#define FRAM_ADDR_LOG_META_B        0x1C0u

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t version;
    uint32_t meta_sequence;

    uint32_t write_index;
    uint32_t count;
    uint32_t next_sequence;

    uint32_t crc32;
} flash_log_meta_t;

typedef struct {
    uint32_t write_index;
    uint32_t count;
    uint32_t next_sequence;

    uint32_t meta_sequence;
    uint8_t  meta_slot;
} flash_log_state_t;

static flash_log_state_t g_log;

static uint32_t flash_log_addr_from_index(uint32_t index)
{
    return FLASH_LOG_BASE_ADDR + (index * FLASH_LOG_RECORD_SIZE);
}

static int flash_log_record_is_erased(const flash_log_record_t *rec)
{
    const uint8_t *p = (const uint8_t *)rec;

    for (uint32_t i = 0; i < sizeof(*rec); i++) {
        if (p[i] != 0xFFu) {
            return 0;
        }
    }

    return 1;
}

static int flash_log_record_is_valid(const flash_log_record_t *rec)
{
    flash_log_record_t tmp;
    uint32_t crc;

    if (flash_log_record_is_erased(rec)) {
        return 0;
    }

    tmp = *rec;
    crc = tmp.crc32;
    tmp.crc32 = 0u;

    return (crc32((const uint8_t *)&tmp, sizeof(tmp)) == crc) ? 1 : 0;
}

static void flash_log_build_record(flash_log_record_t *rec,
                                   uint32_t timestamp,
                                   const measurement_bme280_t *bme,
                                   const measurement_sht40_t *sht)
{
    memset(rec, 0, sizeof(*rec));

    rec->sequence  = g_log.next_sequence;
    rec->timestamp = timestamp;

    if (bme != 0) {
        rec->temp_x100 = bme->temperature;
        rec->hum_x100  = bme->humidity;
        rec->press_pa  = bme->pressure;
    } else {
        rec->temp_x100 = (int32_t)sht->temperature;
        rec->hum_x100  = (uint32_t)sht->humidity;
        rec->press_pa  = 0u;
    }

    rec->crc32 = 0u;
    rec->crc32 = crc32((const uint8_t *)rec, sizeof(*rec));
}

static uint32_t flash_log_oldest_physical_index(void)
{
    if (g_log.count == 0u) {
        return 0u;
    }

    if (g_log.count < FLASH_LOG_CAPACITY) {
        return 0u;
    }

    return g_log.write_index;
}

static int flash_log_raw_read(uint32_t physical_index, flash_log_record_t *rec)
{
    if (!rec) return -1;
    if (physical_index >= FLASH_LOG_CAPACITY) return -1;

    return mx25_read(flash_log_addr_from_index(physical_index),
                     (uint8_t *)rec,
                     sizeof(*rec));
}

static int flash_log_raw_write(uint32_t physical_index, const flash_log_record_t *rec)
{
    if (!rec) return -1;
    if (physical_index >= FLASH_LOG_CAPACITY) return -1;

    return mx25_write(flash_log_addr_from_index(physical_index),
                      (const uint8_t *)rec,
                      sizeof(*rec));
}

static void flash_log_meta_fill(flash_log_meta_t *meta)
{
    memset(meta, 0, sizeof(*meta));

    meta->magic         = FLASH_LOG_META_MAGIC;
    meta->version       = FLASH_LOG_META_VERSION;
    meta->meta_sequence = g_log.meta_sequence;

    meta->write_index   = g_log.write_index;
    meta->count         = g_log.count;
    meta->next_sequence = g_log.next_sequence;

    meta->crc32 = 0u;
    meta->crc32 = crc32((const uint8_t *)meta, sizeof(*meta));
}

static int flash_log_meta_is_valid(const flash_log_meta_t *meta)
{
    flash_log_meta_t tmp;
    uint32_t crc;

    if (!meta) return 0;
    if (meta->magic != FLASH_LOG_META_MAGIC) return 0;
    if (meta->version != FLASH_LOG_META_VERSION) return 0;
    if (meta->write_index >= FLASH_LOG_CAPACITY) return 0;
    if (meta->count > FLASH_LOG_CAPACITY) return 0;
    if (meta->next_sequence == 0u || meta->next_sequence == 0xFFFFFFFFu) return 0;

    tmp = *meta;
    crc = tmp.crc32;
    tmp.crc32 = 0u;

    return (crc32((const uint8_t *)&tmp, sizeof(tmp)) == crc) ? 1 : 0;
}

static int flash_log_meta_read(uint16_t addr, flash_log_meta_t *meta)
{
    if (!meta) return -1;
    return fm24cl16b_read(addr, (uint8_t *)meta, sizeof(*meta));
}

static int flash_log_meta_write(uint16_t addr, const flash_log_meta_t *meta)
{
    if (!meta) return -1;
    return fm24cl16b_write(addr, (const uint8_t *)meta, sizeof(*meta));
}

static int flash_log_state_load(void)
{
    flash_log_meta_t a;
    flash_log_meta_t b;
    int valid_a;
    int valid_b;

    if (flash_log_meta_read(FRAM_ADDR_LOG_META_A, &a) != 1) return -1;
    if (flash_log_meta_read(FRAM_ADDR_LOG_META_B, &b) != 1) return -1;

    valid_a = flash_log_meta_is_valid(&a);
    valid_b = flash_log_meta_is_valid(&b);

    if (!valid_a && !valid_b) {
        return -1;
    }

    if (valid_a && (!valid_b || (a.meta_sequence >= b.meta_sequence))) {
        g_log.write_index   = a.write_index;
        g_log.count         = a.count;
        g_log.next_sequence = a.next_sequence;
        g_log.meta_sequence = a.meta_sequence;
        g_log.meta_slot     = 0u;
    } else {
        g_log.write_index   = b.write_index;
        g_log.count         = b.count;
        g_log.next_sequence = b.next_sequence;
        g_log.meta_sequence = b.meta_sequence;
        g_log.meta_slot     = 1u;
    }

    return 1;
}

static int flash_log_state_save(void)
{
    flash_log_meta_t meta;
    uint16_t addr;

    g_log.meta_sequence++;

    g_log.meta_slot ^= 1u;
    addr = (g_log.meta_slot == 0u) ? FRAM_ADDR_LOG_META_A : FRAM_ADDR_LOG_META_B;

    flash_log_meta_fill(&meta);

    return flash_log_meta_write(addr, &meta);
}

static void flash_log_state_set_defaults(void)
{
    g_log.write_index   = 0u;
    g_log.count         = 0u;
    g_log.next_sequence = 1u;

    g_log.meta_sequence = 0u;
    g_log.meta_slot     = 0u;
}

int flash_log_init(void)
{
    if (mx25_init() != 1) {
        return -1;
    }

    if (flash_log_state_load() != 1) {
        flash_log_state_set_defaults();

        if (flash_log_state_save() != 1) {
            return -1;
        }
    }

    return 1;
}

int flash_log_clear(void)
{
    for (uint32_t addr = FLASH_LOG_BASE_ADDR;
         addr < (FLASH_LOG_BASE_ADDR + FLASH_LOG_AREA_SIZE);
         addr += MX25_SECTOR_SIZE) {
        if (mx25_sector_erase_4k(addr) != 1) {
            return -1;
        }
    }

    flash_log_state_set_defaults();

    if (flash_log_state_save() != 1) {
        return -1;
    }

    return 1;
}

int flash_log_append(uint32_t timestamp,
                     const measurement_bme280_t *bme,
                     const measurement_sht40_t *sht)
{
    flash_log_record_t rec;
    uint32_t addr;
    uint32_t next_index;

    if ((bme == 0) && (sht == 0)) {
        return -1;
    }

    flash_log_build_record(&rec, timestamp, bme, (bme != 0) ? 0 : sht);

    addr = flash_log_addr_from_index(g_log.write_index);

    if ((addr % MX25_SECTOR_SIZE) == 0u) {
        if (mx25_sector_erase_4k(addr) != 1) {
            return -1;
        }
    }

    if (flash_log_raw_write(g_log.write_index, &rec) != 1) {
        return -1;
    }

    next_index = g_log.write_index + 1u;
    if (next_index >= FLASH_LOG_CAPACITY) {
        next_index = 0u;
    }

    g_log.write_index = next_index;

    if (g_log.count < FLASH_LOG_CAPACITY) {
        g_log.count++;
    }

    g_log.next_sequence++;

    if (flash_log_state_save() != 1) {
        return -1;
    }

    return 1;
}

int flash_log_read_oldest(uint32_t index, flash_log_record_t *rec)
{
    uint32_t oldest;
    uint32_t physical;

    if (!rec) return -1;
    if (index >= g_log.count) return -1;

    oldest = flash_log_oldest_physical_index();
    physical = oldest + index;

    if (physical >= FLASH_LOG_CAPACITY) {
        physical -= FLASH_LOG_CAPACITY;
    }

    if (flash_log_raw_read(physical, rec) != 1) {
        return -1;
    }

    return flash_log_record_is_valid(rec) ? 1 : -1;
}

int flash_log_read_latest(uint32_t index, flash_log_record_t *rec)
{
    uint32_t latest;
    uint32_t physical;

    if (!rec) return -1;
    if (index >= g_log.count) return -1;

    latest = (g_log.write_index == 0u) ? (FLASH_LOG_CAPACITY - 1u)
                                       : (g_log.write_index - 1u);

    if (latest >= index) {
        physical = latest - index;
    } else {
        physical = FLASH_LOG_CAPACITY - (index - latest);
    }

    if (flash_log_raw_read(physical, rec) != 1) {
        return -1;
    }

    return flash_log_record_is_valid(rec) ? 1 : -1;
}

uint32_t flash_log_count(void)
{
    return g_log.count;
}

uint32_t flash_log_capacity(void)
{
    return FLASH_LOG_CAPACITY;
}