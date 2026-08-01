#include "pcf8563t.h"
#include "i2c.h"

#define PCF8563T_ADDRESS 0x51U
#define REG_CTRL1         0x00
#define REG_CTRL2         0x01
#define REG_SECONDS       0x02
#define REG_MINUTES       0x03
#define REG_HOURS         0x04
#define REG_DAY           0x05
#define REG_WEEKDAY       0x06
#define REG_MONTH         0x07
#define REG_YEAR          0x08
#define REG_ALRM_MIN      0x09
#define REG_ALRM_HOUR     0x0A
#define REG_ALRM_DAY      0x0B
#define REG_ALRM_WDAY     0x0C
#define REG_CLKOUT        0x0D

static uint8_t bcd_to_bin(uint8_t bcd) {
    return (bcd >> 4) * 10U + (bcd & 0x0FU);
}

static uint8_t bin_to_bcd(uint8_t bin) {
    return (uint8_t)(((bin / 10U) << 4) | (bin % 10U));
}

int pcf8563t_init(void) {
    uint8_t control_status1[] = {REG_CTRL1, 0x00};
    uint8_t control_status2[] = {REG_CTRL2, 0x00};
    uint8_t timer_1hz[] = {REG_CLKOUT, 0x80 | 0x03};

    if (i2c2_write(PCF8563T_ADDRESS, control_status1, sizeof(control_status1)) != 1U) return -1;
    if (i2c2_write(PCF8563T_ADDRESS, control_status2, sizeof(control_status2)) != 1U) return -1;
    if (i2c2_write(PCF8563T_ADDRESS, timer_1hz, sizeof(timer_1hz)) != 1U) return -1;

    return 1;
}

int pcf8563t_set_datetime(const rtc_datetime_t *datetime) {
    if (!datetime) return -1;

    uint8_t data[7];

    if (datetime->seconds > 59 || datetime->minutes > 59 || datetime->hours > 23 ||
        datetime->day < 1 || datetime->day > 31 ||
        datetime->month < 1 || datetime->month > 12 ||
        datetime->weekday > 6) {
        return -1;
    }

    uint8_t bcd_sec     = bin_to_bcd((uint8_t)datetime->seconds) & 0x7F;
    uint8_t bcd_min     = bin_to_bcd((uint8_t)datetime->minutes) & 0x7F;
    uint8_t bcd_hour    = bin_to_bcd((uint8_t)datetime->hours) & 0x3F;
    uint8_t bcd_day     = bin_to_bcd((uint8_t)datetime->day) & 0x3F;
    uint8_t bin_wday    = (uint8_t)datetime->weekday & 0x07;
    uint8_t bcd_month   = bin_to_bcd((uint8_t)datetime->month) & 0x1F;

    uint16_t year2 = datetime->year;
    if (year2 >= 2000) {
        year2 -= 2000;
    } else {
        bcd_month |= 0x80;
        year2 = (year2 >= 1900) ? (year2 - 1900) : 0;
    }
    uint8_t bcd_year = bin_to_bcd((uint8_t)year2);


    data[0] = bcd_sec;
    data[1] = bcd_min;
    data[2] = bcd_hour;
    data[3] = bcd_day;
    data[4] = bin_wday;
    data[5] = bcd_month;
    data[6] = bcd_year;

    return (i2c2_write_reg(PCF8563T_ADDRESS, 0x02U, data, sizeof(data)) == 1U) ? 1 : -1;
}


int pcf8563t_get_datetime(rtc_datetime_t *datetime) {
    if (!datetime) return -1;

    uint8_t data[7] = {0};
    rtc_datetime_t temp_datetime;

    uint8_t reg = REG_SECONDS;
    if (i2c2_write(PCF8563T_ADDRESS, &reg, 1) != 1U) return -1;
    if (i2c2_read(PCF8563T_ADDRESS, data, sizeof(data)) != 1U) return -1;

    temp_datetime.seconds = bcd_to_bin(data[0] & 0x7F);
    temp_datetime.minutes = bcd_to_bin(data[1] & 0x7F);
    temp_datetime.hours = bcd_to_bin(data[2] & 0x3F);
    temp_datetime.day = bcd_to_bin(data[3] & 0x3F); 
    temp_datetime.weekday = bcd_to_bin(data[4] & 0x07);
    temp_datetime.month = bcd_to_bin(data[5] & 0x1F);

    if (data[5] & 0x80) {
        temp_datetime.year = 1900 + bcd_to_bin(data[6]);
    } else {
        temp_datetime.year = 2000 + bcd_to_bin(data[6]);
    }

    datetime->seconds = temp_datetime.seconds;
    datetime->minutes = temp_datetime.minutes;
    datetime->hours = temp_datetime.hours;
    datetime->day = temp_datetime.day;
    datetime->weekday = temp_datetime.weekday;
    datetime->month = temp_datetime.month;
    datetime->year = temp_datetime.year;

    return 1;
}
