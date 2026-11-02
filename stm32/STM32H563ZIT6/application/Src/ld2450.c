#include "ld2450.h"
#include <stdio.h>

static uint8_t ld2450_frame[LD2450_FRAME_SIZE];
static uint8_t ld2450_frame_pos = 0U;

static const uint8_t ld2450_header[4] =
{
    0xAAU, 0xFFU, 0x03U, 0x00U
};

int16_t ld2450_coord(uint8_t lo, uint8_t hi)
{
    uint16_t raw = (uint16_t)(lo | (hi << 8));
    return (raw & 0x8000) ? (int16_t)(raw & 0x7FFF)
                          : -(int16_t)(raw & 0x7FFF);
}


uint8_t ld2450_process_rx(const uint8_t *data,uint16_t length, int16_t *x, int16_t *y, int16_t *speed, uint16_t *distance_resolution)
{
    uint8_t frame_ready = 0U;

    for (uint16_t i = 0U; i < length; i++)
    {
        uint8_t byte = data[i];

        if (ld2450_frame_pos < 4U)
        {
            if (byte == ld2450_header[ld2450_frame_pos])
            {
                ld2450_frame[ld2450_frame_pos] = byte;
                ld2450_frame_pos++;
            }
            else
            {
                ld2450_frame_pos = 0U;

                if (byte == ld2450_header[0])
                {
                    ld2450_frame[0] = byte;
                    ld2450_frame_pos = 1U;
                }
            }

            continue;
        }

        ld2450_frame[ld2450_frame_pos] = byte;
        ld2450_frame_pos++;

        if (ld2450_frame_pos == LD2450_FRAME_SIZE)
        {
            if ((ld2450_frame[28] == 0x55U) && (ld2450_frame[29] == 0xCCU))
            {
                if (x != NULL)
                {
                    *x = ld2450_coord(ld2450_frame[4], ld2450_frame[5]);
                }

                if (y != NULL)
                {
                    *y = ld2450_coord(ld2450_frame[6], ld2450_frame[7]);
                }

                if (speed != NULL)
                {
                    *speed = ld2450_coord(ld2450_frame[8], ld2450_frame[9]);
                }

                if (distance_resolution != NULL)
                {
                    *distance_resolution = (uint16_t)ld2450_frame[10] | ((uint16_t)ld2450_frame[11] << 8);
                }

                frame_ready = 1U;
            }
            ld2450_frame_pos = 0U;
        }
    }
    return frame_ready;
}
