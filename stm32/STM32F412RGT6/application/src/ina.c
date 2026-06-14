#include "ina.h"
#include "i2c.h"

#define INA226_REG_CONFIG        0x00
#define INA226_REG_SHUNT_VOLT    0x01
#define INA226_REG_BUS_VOLT      0x02
#define INA226_REG_POWER         0x03
#define INA226_REG_CURRENT       0x04
#define INA226_REG_CALIB         0x05
#define INA226_REG_MASK_ENABLE   0x06
#define INA226_REG_ALERT_LIMIT   0x07

#define INA226_BUS_LSB_uV        1250
#define INA226_SHUNT_LSB_uV      2

static uint8_t  g_addr;
static uint32_t g_current_lsb_uA;
static uint32_t g_rshunt_mOhm;

static int ina_write_u16(uint8_t reg, uint16_t value)
{
    return i2c1_write_u8_u16_dma(g_addr, reg, value);
}

static int ina_read_u16(uint8_t reg, uint16_t *value)
{
    return i2c1_read_u8_u16_dma(g_addr, reg, value);
}

static int ina_read_s16(uint8_t reg, int16_t *value)
{
    uint16_t u;
    if (ina_read_u16(reg, &u) < 0) return -1;
    *value = (int16_t)u;
    return 0;
}

static uint32_t pick_current_lsb_uA(uint32_t max_current_mA)
{
    uint32_t uA = (max_current_mA * 1000UL + 32767UL) / 32768UL;
    if (uA == 0) uA = 1;
    return uA;
}

void ina226_init(uint8_t addr7, uint32_t rshunt_mOhm, uint32_t max_current_mA)
{
    g_addr = addr7;
    g_rshunt_mOhm = rshunt_mOhm;
    g_current_lsb_uA = pick_current_lsb_uA(max_current_mA);

    ina_write_u16(INA226_REG_CONFIG, 0x4127);

    uint32_t denom = g_current_lsb_uA * g_rshunt_mOhm;
    uint16_t cal = (uint16_t)((5120000UL + denom/2) / denom);
    ina_write_u16(INA226_REG_CALIB, cal);
}

int ina226_id(uint16_t *id, uint16_t *cal)
{
    if (ina_read_u16(0xFE, id) < 0) return -1;
    if (ina_read_u16(0x05, cal) < 0) return -2;
    return 0;
}

uint32_t ina226_bus_uV(void)
{
    uint16_t raw;
    if (ina_read_u16(INA226_REG_BUS_VOLT, &raw) < 0) return 0;
    return (uint32_t)raw * INA226_BUS_LSB_uV;
}

int32_t ina226_shunt_uV(void)
{
    int16_t raw;
    if (ina_read_s16(INA226_REG_SHUNT_VOLT, &raw) < 0) return 0;
    return ((int32_t)raw * 25) / 10;
}

int32_t ina226_current_uA(void)
{
    int16_t raw;
    if (ina_read_s16(INA226_REG_CURRENT, &raw) < 0) return 0;
    return (int32_t)raw * (int32_t)g_current_lsb_uA;
}

uint32_t ina226_power_uW(void)
{
    uint16_t raw;
    if (ina_read_u16(INA226_REG_POWER, &raw) < 0) return 0;
    uint32_t power_lsb_uW = 25UL * g_current_lsb_uA;
    return (uint32_t)raw * power_lsb_uW;
}

void ina226_set_overcurrent_mA(uint32_t limit_mA)
{
    uint32_t vshunt_uV = limit_mA * g_rshunt_mOhm;
    uint16_t raw = (uint16_t)((vshunt_uV * 10UL + 12UL) / 25UL);

    ina_write_u16(INA226_REG_MASK_ENABLE, 0x8002);
    ina_write_u16(INA226_REG_ALERT_LIMIT, raw);
}

uint8_t ina226_is_present(void)
{
    uint16_t dummy;
    return ina_read_u16(INA226_REG_CONFIG, &dummy) == 0;
}