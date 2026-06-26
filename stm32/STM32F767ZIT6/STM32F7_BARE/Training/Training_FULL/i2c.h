#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "stm32f7xx.h"

#define I2C2_SLAVE_ADDRESS 0x50U // Example slave address

void i2c2_slave_config(void);
void i2c1_master_config(void);
uint32_t i2c2_get_address_matchode(void);

#define I2C_DIRECTION_WRITE 0x00U

#endif // I2C_H