#include "ld2450.h"
#include <stdio.h>

volatile uint16_t ld2450_rx_old_pos;

uint8_t ld2450_frame_acc[LD2450_FRAME_LEN];
volatile uint16_t ld2450_frame_idx;

uint8_t ld2450_data[LD2450_FRAME_LEN];

#define HEADER1 0xAA
#define HEADER2 0xFF
#define HEADER3 0x03
#define HEADER4 0x00

#define TAIL1 0x55
#define TAIL2 0xCC

int16_t ld2450_coord(uint8_t lo, uint8_t hi)
{
    uint16_t raw = (uint16_t)(lo | (hi << 8));
    return (raw & 0x8000) ? (int16_t)(raw & 0x7FFF)
                          : -(int16_t)(raw & 0x7FFF);
}

void ld2450_process_rx(void)
{
    uint16_t pos = (uint16_t)(uart2_buffer_size - DMA1_Stream5->NDTR);

    while (ld2450_rx_old_pos != pos) {
        uint8_t b = uart2_data[ld2450_rx_old_pos++];
        if (ld2450_rx_old_pos >= uart2_buffer_size) ld2450_rx_old_pos = 0;

        switch (ld2450_frame_idx) {
        case 0:
            if (b == HEADER1) { ld2450_frame_acc[0] = b; ld2450_frame_idx = 1; }
            break;
        case 1:
            if (b == HEADER2)      { ld2450_frame_acc[1] = b; ld2450_frame_idx = 2; }
            else if (b == HEADER1) { ld2450_frame_acc[0] = b; ld2450_frame_idx = 1; }
            else                   { ld2450_frame_idx = 0; }
            break;
        case 2:
            if (b == HEADER3)      { ld2450_frame_acc[2] = b; ld2450_frame_idx = 3; }
            else if (b == HEADER1) { ld2450_frame_acc[0] = b; ld2450_frame_idx = 1; }
            else                   { ld2450_frame_idx = 0; }
            break;
        case 3:
            if (b == HEADER4)      { ld2450_frame_acc[3] = b; ld2450_frame_idx = 4; }
            else if (b == HEADER1) { ld2450_frame_acc[0] = b; ld2450_frame_idx = 1; }
            else                   { ld2450_frame_idx = 0; }
            break;
        default:
            ld2450_frame_acc[ld2450_frame_idx++] = b;

            if (ld2450_frame_idx == LD2450_FRAME_LEN) {
                ld2450_frame_idx = 0;
                if (ld2450_frame_acc[28] == TAIL1 && ld2450_frame_acc[29] == TAIL2) {
                    for (int i = 0; i < LD2450_FRAME_LEN; i++) {
                        ld2450_data[i] = ld2450_frame_acc[i];
                    }
                }
            }
            break;
        }
    }
}
