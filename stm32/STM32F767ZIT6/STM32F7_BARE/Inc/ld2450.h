#ifndef LD2450_H
#define LD2450_H

#include <stdint.h>
#include "stm32f7xx.h"
#include "uart.h"

#define LD2450_FRAME_LEN 30

extern uint8_t ld2450_frame_acc[LD2450_FRAME_LEN];
extern volatile uint16_t ld2450_frame_idx;
extern uint8_t ld2450_data[LD2450_FRAME_LEN];

int16_t ld2450_coord(uint8_t lo, uint8_t hi);
void ld2450_process_rx(void);

#endif // LD2450_H