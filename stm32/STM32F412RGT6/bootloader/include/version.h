#ifndef VERSION_H
#define VERSION_H

#include <stdint.h>

#define BL_VERSION_MAJOR  0
#define BL_VERSION_MINOR  0
#define BL_VERSION_PATCH  1

#define BL_VERSION_U32 ((uint32_t)(BL_VERSION_MAJOR << 16) | (uint32_t)(BL_VERSION_MINOR << 8) | (uint32_t)(BL_VERSION_PATCH))

typedef struct __attribute__((packed)) {
    uint8_t major;
    uint8_t minor;
    uint8_t patch;
    uint8_t reserved;
    char build[4];
} bl_info_t;

typedef struct __attribute__((packed)) {
    uint32_t serial;
    uint8_t  hw_major;
    uint8_t  hw_minor;
    uint8_t  reserved[2];
    char     prod_date[8];
} device_info_t;

static const char BL_BUILD_DATE[] = __DATE__ " " __TIME__;

#endif // VERSION_H