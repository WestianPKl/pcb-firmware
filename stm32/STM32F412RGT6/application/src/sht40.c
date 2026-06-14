#include "sht40.h"
#include "i2c.h"
#include "systick.h"
#include "dma.h"

#define SHT40_ADDR 0x44
#define SHT40_CMD_SINGLE_SHOT_HIGHREP 0xFD
#define SHT40_SERIAL_READ 0x89

static uint8_t sht_crc8(const uint8_t *data, uint8_t len)
{
    uint8_t crc = 0xFF;
    for (uint8_t j = 0; j < len; j++) {
        crc ^= data[j];
        for (uint8_t i = 0; i < 8; i++) {
            crc = (crc & 0x80) ? (uint8_t)((crc << 1) ^ 0x31) : (uint8_t)(crc << 1);
        }
    }
    return crc;
}

uint32_t sht40_read_serial_number(void)
{
    uint8_t cmd = SHT40_SERIAL_READ;
    uint8_t data[6];
    if (i2c1_write_raw_dma(SHT40_ADDR, &cmd, 1) != 1) {
        return 0;
    }
    systick_delay_ms(1);
    if (i2c1_read_raw_dma(SHT40_ADDR, data, 6) != 1) {
        return 0;
    }
    if (data[2] != sht_crc8(data, 2) || data[5] != sht_crc8(&data[3], 2)) {
        return 0;
    }
    uint32_t serial = ((uint32_t)data[0] << 24) | ((uint32_t)data[1] << 16);
    serial |= ((uint32_t)data[3] << 8) | data[4];
    return serial;
}

static uint8_t sht40_read_raw(uint16_t *rawT, uint16_t *rawRH)
{
    if (!rawT || !rawRH) return 1;

    uint8_t cmd = SHT40_CMD_SINGLE_SHOT_HIGHREP;
    if (i2c1_write_raw_dma(SHT40_ADDR, &cmd, 1) != 1) {
        return 2;
    }
    systick_delay_ms(15);
    uint8_t data[6];
    if (i2c1_read_raw_dma(SHT40_ADDR, data, 6) != 1) {
        return 3;
    }
    if (data[2] != sht_crc8(data, 2) || data[5] != sht_crc8(&data[3], 2)) {
        return 4;
    }
    *rawT = ((uint16_t)data[0] << 8) | data[1];
    *rawRH = ((uint16_t)data[3] << 8) | data[4];
    return 0;
}

uint8_t sht40_single_shot_measurement(uint8_t *data)
{
    uint8_t cmd = SHT40_CMD_SINGLE_SHOT_HIGHREP;
    if (i2c1_write_raw_dma(SHT40_ADDR, &cmd, 1) != 1) return 1;
    systick_delay_ms(15);
    if (i2c1_read_raw_dma(SHT40_ADDR, data, 6) != 1) return 2;
    return 0;
}

uint8_t sht40_read_data(float *temp_c, float *rh)
{
    if (!temp_c || !rh) return 1;

    uint16_t rawT, rawRH;
    uint8_t e = sht40_read_raw(&rawT, &rawRH);
    if (e) return e;

    *temp_c = -45.0f + 175.0f * ((float)rawT / 65535.0f);
    float r = -6.0f + 125.0f * ((float)rawRH / 65535.0f);

    if (r < 0.0f) r = 0.0f;
    if (r > 100.0f) r = 100.0f;

    *rh = r;
    return 0;
}

uint8_t sht40_data_read_int(int16_t *temp_c, uint16_t *rh)
{
    if (!temp_c || !rh) return 1;

    uint16_t rawT, rawRH;
    uint8_t e = sht40_read_raw(&rawT, &rawRH);
    if (e) return e;

    int32_t tx100 = -4500;
    tx100 += (int32_t)((17500UL * (uint32_t)rawT + 32767UL) / 65535UL);

    int32_t hx100 = -600;
    hx100 += (int32_t)((12500UL * (uint32_t)rawRH + 32767UL) / 65535UL);

    if (tx100 < -32768) tx100 = -32768;
    if (tx100 >  32767) tx100 =  32767;

    if (hx100 < 0) hx100 = 0;
    if (hx100 > 10000) hx100 = 10000;

    *temp_c = (int16_t)tx100;
    *rh     = (uint16_t)hx100;
    return 0;
}

