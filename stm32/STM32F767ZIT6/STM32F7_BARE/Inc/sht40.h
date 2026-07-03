#ifndef SHT40_H
#define SHT40_H

#include <stdint.h>

typedef struct {
  int16_t temperature;
  uint16_t humidity;
} sht40_data_t;

uint8_t sht40_single_shot_measurement(uint8_t *data);
uint32_t sht40_read_serial_number(void);
uint8_t sht40_read_data(float *temp_c, float *rh);
uint8_t sht40_data_read_int(int16_t *temp_x100, uint16_t *rh_x100);

#endif // SHT40_H
