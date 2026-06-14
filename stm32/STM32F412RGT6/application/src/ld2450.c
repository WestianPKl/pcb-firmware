#include "ld2450.h"

uint8_t           ld2450_rx_buf[LD2450_RX_BUF_SIZE];
volatile uint16_t ld2450_rx_old_pos;
LD2450_Target     ld2450_targets[LD2450_MAX_TARGETS];

static uint8_t  ld2450_frame_acc[LD2450_FRAME_SIZE];
static uint16_t ld2450_frame_idx;

static uint16_t u16_le(const uint8_t *data, uint8_t pos)
{
    return (uint16_t)(data[pos] | ((uint16_t)data[pos + 1] << 8));
}

static int16_t radar_signed(uint16_t raw)
{
    if (raw & 0x8000U) {
        return (int16_t)(raw - 0x8000U);
    }
    return -(int16_t)raw;
}


static uint32_t isqrt32(uint32_t n)
{
    if (n == 0U) {
        return 0U;
    }
    uint32_t x = n;
    uint32_t y = (x + 1U) / 2U;
    while (y < x) {
        x = y;
        y = (x + n / x) / 2U;
    }
    return x;
}

uint8_t ld2450_parse_target(const uint8_t *frame, uint8_t index, LD2450_Target *out)
{
    uint8_t pos = LD2450_DATA_OFFSET + index * LD2450_TARGET_SIZE;

    uint16_t raw_x    = u16_le(frame, pos);
    uint16_t raw_y    = u16_le(frame, (uint8_t)(pos + 2U));
    uint16_t raw_spd  = u16_le(frame, (uint8_t)(pos + 4U));
    uint16_t raw_res  = u16_le(frame, (uint8_t)(pos + 6U));

    if (raw_x == 0U && raw_y == 0U && raw_spd == 0U && raw_res == 0U) {
        out->valid = 0;
        return 0;
    }

    int16_t x     = radar_signed(raw_x);
    int16_t y     = radar_signed(raw_y);
    int16_t speed = radar_signed(raw_spd);

    out->x          = x;
    out->y          = y;
    out->speed      = speed;
    out->resolution = raw_res;
    out->distance   = (uint16_t)isqrt32((uint32_t)((int32_t)x * x + (int32_t)y * y));
    out->valid      = 1;
    return 1;
}

void ld2450_parse_frame(const uint8_t *frame, LD2450_Target targets[LD2450_MAX_TARGETS])
{
    for (uint8_t i = 0; i < LD2450_MAX_TARGETS; i++) {
        ld2450_parse_target(frame, i, &targets[i]);
    }
}

void ld2450_process_rx(uint16_t dma_remaining)
{
    uint16_t pos = (uint16_t)(LD2450_RX_BUF_SIZE - dma_remaining);

    while (ld2450_rx_old_pos != pos) {
        uint8_t b = ld2450_rx_buf[ld2450_rx_old_pos++];
        if (ld2450_rx_old_pos >= LD2450_RX_BUF_SIZE) {
            ld2450_rx_old_pos = 0;
        }

        if (ld2450_frame_idx == 0U && b != 0xAAU) { continue; }
        if (ld2450_frame_idx == 1U && b != 0xFFU) { ld2450_frame_idx = 0U; continue; }
        if (ld2450_frame_idx == 2U && b != 0x03U) { ld2450_frame_idx = 0U; continue; }
        if (ld2450_frame_idx == 3U && b != 0x00U) { ld2450_frame_idx = 0U; continue; }

        ld2450_frame_acc[ld2450_frame_idx++] = b;

        if (ld2450_frame_idx == LD2450_FRAME_SIZE) {
            if (ld2450_frame_acc[28] == 0x55U && ld2450_frame_acc[29] == 0xCCU) {
                ld2450_parse_frame(ld2450_frame_acc, ld2450_targets);
            }
            ld2450_frame_idx = 0U;
        }
    }
}

