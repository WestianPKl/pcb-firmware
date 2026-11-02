#ifndef INA_H
#define INA_H

#include <stdint.h>

#define INA226_ADDR 0x40U

#define INA226_REG_CONFIG 0x00U
#define INA226_REG_SHUNT_VOLTAGE 0x01U
#define INA226_REG_BUS_VOLTAGE 0x02U
#define INA226_REG_POWER 0x03U
#define INA226_REG_CURRENT 0x04U
#define INA226_REG_CALIBRATION 0x05U
#define INA226_REG_MASK_ENABLE 0x06U
#define INA226_REG_ALERT_LIMIT 0x07U
#define INA226_REG_MANUFACTURER_ID 0xFEU
#define INA226_REG_DIE_ID 0xFFU

extern volatile uint16_t ina_manufacturer_id;
extern volatile uint16_t ina_die_id;
extern volatile int32_t ina_current_ma;
extern volatile uint32_t ina_voltage_mv;
extern volatile uint32_t ina_power_mw;
extern volatile int32_t ina_shunt_voltage_uv_x10;
extern volatile uint8_t ina_data_valid;

uint8_t ina_init(uint8_t *calibration);
uint8_t ina_read_current(void);
uint8_t ina_read_voltage(void);
uint8_t ina_read_power(void);
uint8_t ina_read_shunt_voltage(void);
uint8_t ina_update(void);

#endif // INA_H