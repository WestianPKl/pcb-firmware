#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "stm32f7xx.h"

#define I2C1_SDA 9U // PB9
#define I2C1_SCL 8U // PB8

void i2c1_init(void);
uint8_t i2c1_write(uint8_t addr, const uint8_t *buf, uint16_t len);
uint8_t i2c1_read(uint8_t addr, uint8_t *buf, uint16_t len);
uint8_t i2c1_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len);
uint8_t i2c1_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

#endif // I2C_H