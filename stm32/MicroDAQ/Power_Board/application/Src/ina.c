#include "ina.h"
#include "i2c.h"
#include "fault.h"

volatile uint16_t ina_manufacturer_id = 0;
volatile uint16_t ina_die_id = 0;
volatile int32_t ina_current_ma = 0;
volatile uint32_t ina_voltage_mv = 0;
volatile uint32_t ina_power_mw = 0;
volatile int32_t ina_shunt_voltage_uv_x10 = 0;
volatile uint8_t ina_data_valid = 0U;

uint8_t ina_init(uint8_t *calibration)
{
    uint8_t id[2];

    if (i2c1_read_reg(INA226_ADDR, INA226_REG_MANUFACTURER_ID, id, 2U) != 1)
    {
        return 0;
    }
    ina_manufacturer_id = ((uint16_t)id[0] << 8) | (uint16_t)id[1];
    
    if (i2c1_read_reg(INA226_ADDR, INA226_REG_DIE_ID, id, 2U) != 1)
    {
        return 0;
    }
    ina_die_id = ((uint16_t)id[0] << 8) | (uint16_t)id[1];

    if (ina_manufacturer_id != 0x5449U || ina_die_id != 0x2260U)
    {
        fault_set(FAULT_INA);
        return 0;
    }

    if (i2c1_write_reg(INA226_ADDR, INA226_REG_CALIBRATION, calibration, 2U) != 1)
    {
        return 0;
    };

    uint8_t readback[2];
    if (i2c1_read_reg(INA226_ADDR, INA226_REG_CALIBRATION, readback, 2U) != 1U)
    {
        return 0U;
    }

    if ((readback[0] != calibration[0]) || (readback[1] != calibration[1]))
    {
        return 0U;
    }

    return 1;
}

uint8_t ina_read_current(void)
{
    uint8_t data[2];
    if (i2c1_read_reg(INA226_ADDR, INA226_REG_CURRENT, data, 2U) != 1)
    {
        return 0;
    }
    int16_t current_raw = (int16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    ina_current_ma = current_raw;
    return 1;
}

uint8_t ina_read_voltage(void)
{
    uint8_t data[2];
    if (i2c1_read_reg(INA226_ADDR, INA226_REG_BUS_VOLTAGE, data, 2U) != 1)
    {
        return 0;
    }
    uint16_t voltage_raw = ((uint16_t)data[0] << 8) | (uint16_t)data[1];
    ina_voltage_mv = ((uint32_t)voltage_raw * 5U) / 4U;
    return 1;
}

uint8_t ina_read_power(void)
{
    uint8_t data[2];
    if (i2c1_read_reg(INA226_ADDR, INA226_REG_POWER, data, 2U) != 1)
    {
        return 0;
    }
    uint16_t power_raw = ((uint16_t)data[0] << 8) | (uint16_t)data[1];
    ina_power_mw = (uint32_t)power_raw * 25U;
    return 1;
}

uint8_t ina_read_shunt_voltage(void)
{
    uint8_t data[2];
    if (i2c1_read_reg(INA226_ADDR, INA226_REG_SHUNT_VOLTAGE, data, 2U) != 1)
    {
        return 0;
    }
    int16_t shunt_voltage_raw = (int16_t)(((uint16_t)data[0] << 8) | (uint16_t)data[1]);
    ina_shunt_voltage_uv_x10 = (int32_t)shunt_voltage_raw * 25;
    return 1;
}

uint8_t ina_update(void)
{
    static uint8_t initialized = 0U;

    uint8_t calibration[2] = {0x02U, 0x00U};
    uint8_t readback[2];

    ina_data_valid = 0U;

    if (initialized == 0U)
    {
        initialized = ina_init(calibration);
        return 0U;
    }

    if (i2c1_read_reg(INA226_ADDR, INA226_REG_CALIBRATION, readback, 2U) != 1U)
    {
        initialized = 0U;
        return 0U;
    }

    if ((readback[0] != calibration[0]) || (readback[1] != calibration[1]))
    {
        initialized = 0U;
        return 0U;
    }

    uint8_t ok = ina_read_current();

    ok &= ina_read_voltage();
    ok &= ina_read_power();
    ok &= ina_read_shunt_voltage();

    if (ok == 0U)
    {
        initialized = 0U;
        return 0U;
    }

    ina_data_valid = 1U;
    return 1U;
}