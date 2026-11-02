
#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "stm32h5xx.h"

#define I2C1_SCL 8U // PB8
#define I2C1_SDA 9U // PB9

#define I2C2_SCL 1U // PF1
#define I2C2_SDA 0U // PF0

void i2c1_init(void);
uint8_t i2c1_write(uint8_t addr, const uint8_t *buf, uint16_t len);
uint8_t i2c1_read(uint8_t addr, uint8_t *buf, uint16_t len);
uint8_t i2c1_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len);
uint8_t i2c1_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

// uint8_t i2c1_dma_write_start(uint8_t addr, const uint8_t *buf, uint16_t len);
// uint8_t i2c1_dma_read_start(uint8_t addr, uint8_t *buf, uint16_t len);

void i2c2_init(void);

#endif // I2C_H