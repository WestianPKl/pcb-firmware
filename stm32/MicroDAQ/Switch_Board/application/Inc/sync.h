#ifndef SYNC_H
#define SYNC_H

#include <stdint.h>
#include "stm32c5xx.h"

#define SYNC_PIN 8U // PA8

extern volatile uint8_t sync_event;
extern volatile uint32_t sync_counter;

void sync_init(void);

#endif // SYNC_H