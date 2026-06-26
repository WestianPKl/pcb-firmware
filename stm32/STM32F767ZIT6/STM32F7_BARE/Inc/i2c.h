#ifndef I2C_H
#define I2C_H

#include <stdint.h>
#include "stm32f7xx.h"

#define I2C_TIMING 0x00303D5B
#define I2C1_SLAVE_ADDRESS 0x42U
#define I2C_DIRECTION_WRITE 0x00U

#define I2C1_SCL 8U // PB8
#define I2C1_SDA 9U // PB9
#define I2C2_SCL 10U // PB10
#define I2C2_SDA 11U // PB11

#define I2C1_BUF_SIZE 16U

extern volatile uint8_t i2c1_rx_buffer[I2C1_BUF_SIZE];
extern volatile uint8_t i2c1_tx_buffer[I2C1_BUF_SIZE];
extern volatile uint8_t i2c1_rx_idx;
extern volatile uint8_t i2c1_tx_idx;
extern volatile uint8_t i2c1_rx_done;

void i2c1_slave_config(void);

void i2c2_master_config(void);
void i2c2_master_write(uint8_t addr, uint8_t *buf, uint16_t len);
void i2c2_master_read(uint8_t addr, uint8_t *buf, uint16_t len);
uint8_t i2c2_master_wait(void);

uint8_t i2c2_write(uint8_t addr, const uint8_t *buf, uint16_t len);
uint8_t i2c2_read(uint8_t addr, uint8_t *buf, uint16_t len);
uint8_t i2c2_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len);
uint8_t i2c2_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len);

uint32_t i2c2_get_address_matchode(void);

#endif // I2C_H