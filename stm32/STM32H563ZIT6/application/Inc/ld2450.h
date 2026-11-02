
#ifndef LD2450_H
#define LD2450_H

#include <stdint.h>
#include "stm32h5xx.h"

#define LD2450_FRAME_SIZE 30U

int16_t ld2450_coord(uint8_t lo, uint8_t hi);
uint8_t ld2450_process_rx(const uint8_t *data, uint16_t length, int16_t *x, int16_t *y, int16_t *speed, uint16_t *distance_resolution);

#endif // LD2450_H