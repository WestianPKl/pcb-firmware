#ifndef AI_H
#define AI_H

#include <stdint.h>

uint8_t ai_init(void);
uint8_t ai_run(float input_value, uint8_t *class_id);
uint8_t ai_temperature_x100_run(int16_t temperature, uint8_t *class_id);

#endif // AI_H