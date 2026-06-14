#ifndef LD2450_H
#define LD2450_H

#include <stdint.h>

#define LD2450_MAX_TARGETS  3
#define LD2450_FRAME_SIZE   30
#define LD2450_DATA_OFFSET  4
#define LD2450_TARGET_SIZE  8
#define LD2450_RX_BUF_SIZE  64

typedef struct {
    int16_t  x;
    int16_t  y;
    int16_t  speed;
    uint16_t distance;
    uint16_t resolution;
    uint8_t  valid;
} LD2450_Target;

extern uint8_t           ld2450_rx_buf[LD2450_RX_BUF_SIZE];
extern volatile uint16_t ld2450_rx_old_pos;
extern LD2450_Target     ld2450_targets[LD2450_MAX_TARGETS];

void    ld2450_process_rx(uint16_t dma_remaining);
void    ld2450_parse_frame(const uint8_t *frame, LD2450_Target targets[LD2450_MAX_TARGETS]);
uint8_t ld2450_parse_target(const uint8_t *frame, uint8_t index, LD2450_Target *out);

#endif // LD2450_H