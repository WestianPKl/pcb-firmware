#ifndef INA_H
#define INA_H

#include <stdint.h>
#include "stm32f4xx.h"

void ina226_init(uint8_t addr7, uint32_t rshunt_mOhm, uint32_t max_current_mA);
uint32_t ina226_bus_uV(void);
int32_t ina226_shunt_uV(void);
int32_t ina226_current_uA(void);
uint32_t ina226_power_uW(void);
void ina226_set_overcurrent_mA(uint32_t limit_mA);
uint8_t ina226_is_present(void);

int ina226_id(uint16_t *id, uint16_t *cal);


#endif // INA_H