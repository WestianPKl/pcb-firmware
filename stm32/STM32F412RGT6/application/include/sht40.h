#ifndef SHT40_H
#define SHT40_H

#include <stdint.h>

uint8_t sht40_single_shot_measurement(uint8_t *data);
uint32_t sht40_read_serial_number(void);
uint8_t sht40_read_data(float *temp_c, float *rh);
uint8_t sht40_data_read_int(int16_t *temp_c, uint16_t *rh);

#endif // SHT40_H