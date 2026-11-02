#include "version.h"
#include "support.h"

uint8_t board_info_valid(void)
{
    const board_info_t *info = board_info_get();

    if (info->magic != INFO_MAGIC)
    {
        return 0U;
    }

    if (info->struct_version != BOARD_INFO_STRUCT_VERSION)
    {
        return 0U;
    }

    uint32_t crc = crc32((const uint8_t *)info, BOARD_INFO_CRC_LENGTH);

    if (crc != info->crc32)
    {
        return 0U;
    }

    return 1U;
}