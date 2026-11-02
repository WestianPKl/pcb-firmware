#include "bme280.h"
#include "spi.h"
#include "support.h"
#include "timer.h"

#define REG_ID        0xD0
#define REG_RESET     0xE0
#define REG_CTRL_HUM  0xF2
#define REG_CTRL_MEAS 0xF4
#define REG_CONFIG    0xF5
#define REG_PRESS_MSB 0xF7

#define REG_DIG_T1 0x88
#define REG_DIG_T2 0x8A
#define REG_DIG_T3 0x8C
#define REG_DIG_P1 0x8E
#define REG_DIG_P2 0x90
#define REG_DIG_P3 0x92
#define REG_DIG_P4 0x94
#define REG_DIG_P5 0x96
#define REG_DIG_P6 0x98
#define REG_DIG_P7 0x9A
#define REG_DIG_P8 0x9C
#define REG_DIG_P9 0x9E

#define REG_DIG_H1 0xA1
#define REG_DIG_H2 0xE1

#define OSRS_1X      0x01
#define MODE_SLEEP   0x00
#define MODE_FORCED  0x01

typedef struct {
    uint16_t dig_T1;
    int16_t  dig_T2;
    int16_t  dig_T3;

    uint16_t dig_P1;
    int16_t  dig_P2;
    int16_t  dig_P3;
    int16_t  dig_P4;
    int16_t  dig_P5;
    int16_t  dig_P6;
    int16_t  dig_P7;
    int16_t  dig_P8;
    int16_t  dig_P9;

    uint8_t  dig_H1;
    int16_t  dig_H2;
    uint8_t  dig_H3;
    int16_t  dig_H4;
    int16_t  dig_H5;
    int8_t   dig_H6;
} bme280_calib_t;

static bme280_calib_t calib;
static int32_t t_fine = 0;

#define BME_XFER_MAX_DATA 32U

static uint8_t bme280_spi_transfer_dma(const uint8_t *tx, uint8_t *rx, uint16_t length)
{
    if (spi1_dma_transfer(tx, rx, length, 1U) == 0U)
    {
        return 0U;
    }

    while ((spi1_dma_done == 0U) && (spi1_dma_error == 0U)){}

    if (spi1_dma_error != 0U)
    {
        return 0U;
    }

    return 1U;
}

static uint8_t bme280_read_registers(uint8_t reg, uint8_t *data, uint32_t size)
{
    if (!data || size == 0U) return 0U;
    if (size > BME_XFER_MAX_DATA) return 0U;

    uint32_t len = size + 1U;
    uint8_t txbuf[1U + BME_XFER_MAX_DATA];
    uint8_t rxbuf[1U + BME_XFER_MAX_DATA];

    txbuf[0] = reg | 0x80U;
    for (uint32_t i = 1; i < len; i++) txbuf[i] = 0x00U;

    if (bme280_spi_transfer_dma(txbuf, rxbuf, (uint16_t)len) == 0U)
    {
        return 0U;
    }

    for (uint32_t i = 0; i < size; i++) data[i] = rxbuf[i + 1U];

    return 1U;
}

static uint8_t bme280_write_register(uint8_t reg, uint8_t value)
{
    uint8_t txbuf[2];
    uint8_t rxbuf[2];

    txbuf[0] = reg & 0x7FU;
    txbuf[1] = value;

    if (bme280_spi_transfer_dma(txbuf, rxbuf, 2U) == 0U)
    {
        return 0U;
    }

    return 1U;
}

static int16_t sign_extend_12(int16_t v)
{
    if (v & 0x0800) v |= (int16_t)0xF000;
    return v;
}

static uint8_t bme280_read_calibration_data(void)
{
    uint8_t data[26];

    if (bme280_read_registers(REG_DIG_T1, data, 24) == 0U) return 0U;
    calib.dig_T1 = (uint16_t)(data[0] | (data[1] << 8));
    calib.dig_T2 = (int16_t)(data[2] | (data[3] << 8));
    calib.dig_T3 = (int16_t)(data[4] | (data[5] << 8));
    calib.dig_P1 = (uint16_t)(data[6] | (data[7] << 8));
    calib.dig_P2 = (int16_t)(data[8] | (data[9] << 8));
    calib.dig_P3 = (int16_t)(data[10] | (data[11] << 8));
    calib.dig_P4 = (int16_t)(data[12] | (data[13] << 8));
    calib.dig_P5 = (int16_t)(data[14] | (data[15] << 8));
    calib.dig_P6 = (int16_t)(data[16] | (data[17] << 8));
    calib.dig_P7 = (int16_t)(data[18] | (data[19] << 8));
    calib.dig_P8 = (int16_t)(data[20] | (data[21] << 8));
    calib.dig_P9 = (int16_t)(data[22] | (data[23] << 8));

    if (bme280_read_registers(REG_DIG_H1, &data[0], 1) == 0U) return 0U;
    calib.dig_H1 = data[0];

    if (bme280_read_registers(REG_DIG_H2, &data[0], 7) == 0U) return 0U;
    calib.dig_H2 = (int16_t)(data[0] | (data[1] << 8));
    calib.dig_H3 = data[2];

    int16_t h4 = (int16_t)((((int16_t)data[3]) << 4) | (data[4] & 0x0F));
    int16_t h5 = (int16_t)((((int16_t)data[5]) << 4) | (data[4] >> 4));

    calib.dig_H4 = sign_extend_12(h4);
    calib.dig_H5 = sign_extend_12(h5);

    calib.dig_H6 = (int8_t)data[6];
    
    return 1U;
}

uint8_t bme280_read_id(void)
{
    uint8_t id = 0;

    if (bme280_read_registers(REG_ID, &id, 1U) == 0U)
    {
        return 0U;
    }
    
    return id;
}

uint8_t bme280_init(void)
{
    if (bme280_write_register(REG_RESET, 0xB6) == 0U)
    {
        return 0U;
    }

    delay_10us(10U);

    if (bme280_read_calibration_data() == 0U) return 0U;

    if (bme280_write_register(REG_CTRL_HUM, OSRS_1X) == 0U) return 0U;
    if (bme280_write_register(REG_CONFIG, (0x05U << 5) | (0x00U << 2) | 0x00U) == 0U) return 0U;
    if (bme280_write_register(REG_CTRL_MEAS, (OSRS_1X << 5) | (OSRS_1X << 2) | MODE_SLEEP) == 0U) return 0U;

    return 1U;
}

uint8_t bme280_trigger_forced(void)
{
    if (bme280_write_register(REG_CTRL_HUM, OSRS_1X) == 0U) return 0U;
    if (bme280_write_register(REG_CTRL_MEAS, (OSRS_1X << 5) | (OSRS_1X << 2) | MODE_FORCED) == 0U) return 0U;
    return 1U;
}

static uint8_t bme280_read_raw(int32_t *adc_P, int32_t *adc_T, int32_t *adc_H)
{
    if (!adc_P || !adc_T || !adc_H) return 0U;

    uint8_t data[8];
    if (bme280_read_registers(REG_PRESS_MSB, data, 8) == 0U) return 0U;

    *adc_P = (int32_t)((((uint32_t)data[0]) << 12) | (((uint32_t)data[1]) << 4) | (((uint32_t)data[2]) >> 4));
    *adc_T = (int32_t)((((uint32_t)data[3]) << 12) | (((uint32_t)data[4]) << 4) | (((uint32_t)data[5]) >> 4));
    *adc_H = (int32_t)((((uint32_t)data[6]) << 8)  | ((uint32_t)data[7]));
    return 1U;
}

static int32_t bme280_compensate_T_x100(int32_t adc_T)
{
    int32_t var1, var2, T;

    var1 = ((((adc_T >> 3) - ((int32_t)calib.dig_T1 << 1))) * ((int32_t)calib.dig_T2)) >> 11;
    var2 = (((((adc_T >> 4) - ((int32_t)calib.dig_T1)) * ((adc_T >> 4) - ((int32_t)calib.dig_T1))) >> 12) *
            ((int32_t)calib.dig_T3)) >> 14;

    t_fine = var1 + var2;
    T = (t_fine * 5 + 128) >> 8;
    return T;
}

static uint32_t bme280_compensate_H_x1024(int32_t adc_H)
{
    int32_t v_x1_u32r;

    v_x1_u32r = (t_fine - ((int32_t)76800));
    v_x1_u32r = (((((adc_H << 14) - (((int32_t)calib.dig_H4) << 20) - (((int32_t)calib.dig_H5) * v_x1_u32r)) +
                  ((int32_t)16384)) >> 15) *
                (((((((v_x1_u32r * ((int32_t)calib.dig_H6)) >> 10) * (((v_x1_u32r * ((int32_t)calib.dig_H3)) >> 11) +
                   ((int32_t)32768))) >> 10) + ((int32_t)2097152)) * ((int32_t)calib.dig_H2) + 8192) >> 14));

    v_x1_u32r = (v_x1_u32r - (((((v_x1_u32r >> 15) * (v_x1_u32r >> 15)) >> 7) * ((int32_t)calib.dig_H1)) >> 4));

    if (v_x1_u32r < 0) v_x1_u32r = 0;
    if (v_x1_u32r > 419430400) v_x1_u32r = 419430400;

    return (uint32_t)(v_x1_u32r >> 12);
}

static uint32_t bme280_compensate_P(int32_t adc_P)
{
    int64_t var1, var2, p;

    var1 = ((int64_t)t_fine) - 128000;
    var2 = var1 * var1 * (int64_t)calib.dig_P6;
    var2 = var2 + ((var1 * (int64_t)calib.dig_P5) << 17);
    var2 = var2 + (((int64_t)calib.dig_P4) << 35);
    var1 = ((var1 * var1 * (int64_t)calib.dig_P3) >> 8) + ((var1 * (int64_t)calib.dig_P2) << 12);
    var1 = (((((int64_t)1) << 47) + var1)) * ((int64_t)calib.dig_P1) >> 33;

    if (var1 == 0) return 0U;

    p = 1048576 - adc_P;
    p = (((p << 31) - var2) * 3125) / var1;
    var1 = (((int64_t)calib.dig_P9) * (p >> 13) * (p >> 13)) >> 25;
    var2 = (((int64_t)calib.dig_P8) * p) >> 19;

    p = ((p + var1 + var2) >> 8) + (((int64_t)calib.dig_P7) << 4);
    return (uint32_t)p;
}

uint8_t bme280_read_data(int32_t *temp_x100, uint32_t *hum_x100, uint32_t *press_pa)
{
    if (!temp_x100 || !hum_x100 || !press_pa) return 0U;

    int32_t adc_P, adc_T, adc_H;
    uint32_t hum_x1024;
    uint32_t press_q24_8;

    if (bme280_read_raw(&adc_P, &adc_T, &adc_H)==0) return 0U;

    *temp_x100 = bme280_compensate_T_x100(adc_T);

    hum_x1024 = bme280_compensate_H_x1024(adc_H);
    if (hum_x1024 > 102400U) hum_x1024 = 102400U;
    *hum_x100 = (hum_x1024 * 100U) / 1024U;

    press_q24_8 = bme280_compensate_P(adc_P);
    *press_pa = press_q24_8 / 256U;

    return 1U;
}