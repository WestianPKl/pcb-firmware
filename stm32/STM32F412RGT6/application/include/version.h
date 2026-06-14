#ifndef VERSION_H
#define VERSION_H

#include <stdint.h>

#define DEV_ADDR        0xB2

#define FW_VERSION_MAJOR  0
#define FW_VERSION_MINOR  0
#define FW_VERSION_PATCH  1

#define FW_VERSION_U32 ((uint32_t)(((uint32_t)FW_VERSION_MAJOR << 16) | \
                                   ((uint32_t)FW_VERSION_MINOR << 8)  | \
                                   ((uint32_t)FW_VERSION_PATCH)))

#define FW_BUILD_DATE "2026-02-01"

#define INFO_ADDR  0x080FF800U
#define INFO_MAGIC 0x494E464FU

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint32_t serial;
    uint8_t  hw_major;
    uint8_t  hw_minor;
    uint8_t  reserved[2];
    char     prod_date[10];
} device_info_t;

static inline const device_info_t* device_info_get(void)
{
    return (const device_info_t*)INFO_ADDR;
}

#endif // VERSION_H