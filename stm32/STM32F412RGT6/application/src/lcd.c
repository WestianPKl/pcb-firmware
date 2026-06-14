#include "lcd.h"
#include "i2c.h"
#include "systick.h"

#define LCD_I2C_ADDR 0x27 
#define LCD_RS   0x01
#define LCD_RW   0x02
#define LCD_E    0x04
#define LCD_BL   0x08
#define LCD_CLEARDISPLAY   0x01
#define LCD_RETURNHOME     0x02
#define LCD_ENTRYMODESET   0x04
#define LCD_DISPLAYCONTROL 0x08
#define LCD_FUNCTIONSET    0x20
#define LCD_SETDDRAMADDR   0x80
#define LCD_ENTRYLEFT      0x02
#define LCD_DISPLAYON      0x04
#define LCD_2LINE          0x08

static uint8_t lcd_present = 1;
static uint8_t lcd_backlight_mask = LCD_BL;

static int8_t lcd_i2c_write(uint8_t data)
{
    if (!lcd_present) return -1;
    int r = i2c1_write_raw(LCD_I2C_ADDR, &data, 1);
    if (r != 1) lcd_present = 0;
    return (int8_t)r;
}

static inline void lcd_delay_short(void)
{
    for (volatile int i = 0; i < 200; i++) { __NOP(); }
}

static void lcd_pulse_enable(uint8_t data)
{
    if (!lcd_present) return;
    if (lcd_i2c_write(data | LCD_E) != 1) return;
    lcd_delay_short();
    (void)lcd_i2c_write(data & (uint8_t)~LCD_E);
    lcd_delay_short();
}

static void lcd_write4bits(uint8_t nibble, uint8_t rs)
{
    uint8_t data = (uint8_t)((nibble & 0x0F) << 4);
    if (rs) data |= LCD_RS;
    data |= lcd_backlight_mask;
    lcd_pulse_enable(data);
}

static void lcd_write8bits(uint8_t byte, uint8_t rs)
{
    lcd_write4bits((uint8_t)(byte >> 4), rs);
    lcd_write4bits((uint8_t)(byte & 0x0F), rs);
}

static void lcd_command(uint8_t cmd)
{
    lcd_write8bits(cmd, 0);
    if (cmd == LCD_CLEARDISPLAY || cmd == LCD_RETURNHOME) {
        systick_delay_ms(2);
    } else {
        lcd_delay_short();
    }
}

uint8_t lcd_is_present(void)
{
    return lcd_present;
}

void lcd_mark_present(uint8_t present)
{
    lcd_present = present ? 1U : 0U;
}

void lcd_init(void)
{
    lcd_present = 1;
    lcd_backlight_mask = LCD_BL;

    if (lcd_i2c_write(lcd_backlight_mask) != 1) {
        lcd_present = 0;
        return;
    }

    systick_delay_ms(50);

    lcd_write4bits(0x03, 0);
    systick_delay_ms(5);

    lcd_write4bits(0x03, 0);
    systick_delay_ms(1);

    lcd_write4bits(0x03, 0);
    systick_delay_ms(1);

    lcd_write4bits(0x02, 0);
    systick_delay_ms(1);

    lcd_command(LCD_FUNCTIONSET | LCD_2LINE);
    lcd_command(LCD_DISPLAYCONTROL | LCD_DISPLAYON);
    lcd_command(LCD_CLEARDISPLAY);
    lcd_command(LCD_ENTRYMODESET | LCD_ENTRYLEFT);
}

void lcd_set_cursor(uint8_t col, uint8_t row)
{
    if (!lcd_present) return;
    const uint8_t row_offsets[] = {0x00, 0x40};

    if (row > 1) row = 1;
    lcd_command((uint8_t)(LCD_SETDDRAMADDR | (col + row_offsets[row])));
}

void lcd_send_string(const char *str)
{
    if (!lcd_present) return;
    while (*str) {
        lcd_write8bits((uint8_t)(*str++), 1);
    }
}

void lcd_send_decimal(int32_t num, uint8_t digits)
{
    if (!lcd_present) return;
    char buf[12];
    int idx = 0;
    int32_t n = num;

    if (num < 0) {
        buf[idx++] = '-';
        n = -n;
    }

    char temp[10];
    int temp_idx = 0;

    do {
        temp[temp_idx++] = (char)('0' + (n % 10));
        n /= 10;
    } while (n > 0 && temp_idx < 10);

    while (temp_idx < digits) {
        temp[temp_idx++] = '0';
    }

    for (int i = temp_idx - 1; i >= 0; i--) {
        buf[idx++] = temp[i];
    }
    buf[idx] = '\0';

    lcd_send_string(buf);
}

void lcd_send_hex(uint32_t num, uint8_t digits)
{
    if (!lcd_present) return;
    char buf[9];
    int idx = 0;

    for (int i = (digits > 8 ? 7 : digits - 1); i >= 0; i--) {
        uint8_t nibble = (num >> (i * 4)) & 0x0F;
        buf[idx++] = (nibble < 10) ? ('0' + nibble) : ('A' + nibble - 10);
    }
    buf[idx] = '\0';

    lcd_send_string(buf);
}

void lcd_send_fixed_x100(int32_t value_x100)
{
    if (!lcd_present) return;
    int32_t ipart = value_x100 / 100;
    int32_t fpart = value_x100 % 100;
    if (fpart < 0) fpart = -fpart;

    lcd_send_decimal(ipart, 1);
    lcd_send_string(".");
    lcd_send_decimal(fpart, 2);
}

static int32_t x100_to_x10_round(int32_t v_x100)
{
    if (v_x100 >= 0) return (v_x100 + 5) / 10;
    else             return (v_x100 - 5) / 10;
}

static void lcd_send_fixed_x10(int32_t v_x10, uint8_t int_digits)
{
    if (!lcd_present) return;
    int32_t ip = v_x10 / 10;
    int32_t fp = v_x10 % 10;
    if (fp < 0) fp = -fp;

    lcd_send_decimal(ip, int_digits);
    lcd_send_string(".");
    lcd_send_decimal(fp, 1);
}

void lcd_send_temp_1dp_from_x100(int16_t t_x100)
{
    if (!lcd_present) return;
    int32_t t = t_x100;
    if (t < -5000) t = -5000;
    if (t > 12000) t = 12000;

    int32_t t_x10 = x100_to_x10_round(t);
    lcd_send_fixed_x10(t_x10, 1);
}

void lcd_send_hum_1dp_from_x100(uint16_t rh_x100)
{
    if (!lcd_present) return;
    uint32_t rh = rh_x100;
    if (rh > 10000U) rh = 10000U;

    int32_t rh_x10 = (int32_t)((rh + 5U) / 10U);
    int32_t ip = rh_x10 / 10;

    uint8_t digits = (ip >= 100) ? 3 : 2;
    lcd_send_fixed_x10(rh_x10, digits);
}

void lcd_send_press_int_from_q24_8(uint32_t p_q24_8)
{
    if (!lcd_present) return;
    uint32_t p_hpa = (p_q24_8 + 128U) / 256U;
    lcd_send_decimal((int32_t)p_hpa, 5);
}

void lcd_backlight(uint8_t state)
{
    if (!lcd_present) return;
    lcd_backlight_mask = state ? LCD_BL : 0x00;
    uint8_t data = lcd_backlight_mask;
    lcd_i2c_write(data);
}

void lcd_clear(void)
{
    if (!lcd_present) return;
    lcd_command(LCD_CLEARDISPLAY);
}

