#ifndef BME280_H
#define BME280_H

#include <stdint.h>

typedef struct {
  uint8_t valid;
  int32_t temperature;
  uint32_t humidity;
  uint32_t pressure;
} bme280_data_t;

uint8_t bme280_init(void);
uint8_t bme280_trigger_forced(void);
uint8_t bme280_read_id(void);
uint8_t bme280_read_data(int32_t *temp_x100, uint32_t *hum_x100, uint32_t *press_pa);

#endif // BME280_H