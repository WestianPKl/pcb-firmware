#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "stm32c5xx.h"

#define I2C1_SCL 8U // PA8
#define I2C1_SDA 7U // PB7

typedef struct
{
    uint32_t clock_frequency;
    uint8_t addressing_mode;
} i2c_config_t;

void i2c1_init(void);
uint8_t i2c1_config(const i2c_config_t *config);
uint8_t i2c1_write(uint16_t addr, const uint8_t *buf, uint16_t len);
uint8_t i2c1_read(uint16_t addr, uint8_t *buf, uint16_t len);
uint8_t i2c1_write_reg(uint16_t addr, uint8_t reg, const uint8_t *buf, uint16_t len);
uint8_t i2c1_read_reg(uint16_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

#endif // I2C_H