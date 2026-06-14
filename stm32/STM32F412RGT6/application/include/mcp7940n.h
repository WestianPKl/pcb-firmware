#ifndef MCP7940N_H
#define MCP7940N_H

#include <stdint.h>

#define MCP7940N_ADDR              0x6F

#define MCP7940N_REG_RTCSEC        0x00
#define MCP7940N_REG_RTCMIN        0x01
#define MCP7940N_REG_RTCHOUR       0x02
#define MCP7940N_REG_RTCWKDAY      0x03
#define MCP7940N_REG_RTCDATE       0x04
#define MCP7940N_REG_RTCMTH        0x05
#define MCP7940N_REG_RTCYEAR       0x06
#define MCP7940N_REG_CONTROL       0x07
#define MCP7940N_REG_OSCTRIM       0x08

#define MCP7940N_REG_ALM0SEC       0x0A
#define MCP7940N_REG_ALM1SEC       0x11

#define MCP7940N_REG_PWRDNMIN      0x18
#define MCP7940N_REG_SRAM_START    0x20
#define MCP7940N_REG_SRAM_END      0x5F
#define MCP7940N_SRAM_SIZE         64U

#define MCP7940N_BIT_ST            0x80
#define MCP7940N_BIT_VBATEN        0x08
#define MCP7940N_BIT_PWRFAIL       0x10
#define MCP7940N_BIT_OSCRUN        0x20

#define MCP7940N_BIT_OUT       0x80
#define MCP7940N_BIT_SQWEN     0x40
#define MCP7940N_BIT_ALM1EN    0x20
#define MCP7940N_BIT_ALM0EN    0x10
#define MCP7940N_BIT_EXTOSC    0x08
#define MCP7940N_BIT_CRSTRIM   0x04
#define MCP7940N_BIT_SQWFS1    0x02
#define MCP7940N_BIT_SQWFS0    0x01

typedef struct {
    uint8_t sec;    // 0..59
    uint8_t min;    // 0..59
    uint8_t hour;   // 0..23
    uint8_t wday;   // 1..7
    uint8_t mday;   // 1..31
    uint8_t month;  // 1..12
    uint8_t year;   // 0..99
} mcp7940n_datetime_t;

int mcp7940n_init(uint8_t enable_battery_backup);
int mcp7940n_get_datetime(mcp7940n_datetime_t *dt);
int mcp7940n_set_datetime(const mcp7940n_datetime_t *dt);

int mcp7940n_read_reg(uint8_t reg, uint8_t *value);
int mcp7940n_write_reg(uint8_t reg, uint8_t value);
int mcp7940n_read_regs(uint8_t reg, uint8_t *data, uint16_t len);
int mcp7940n_write_regs(uint8_t reg, const uint8_t *data, uint16_t len);

int mcp7940n_read_sram(uint8_t offset, uint8_t *data, uint8_t len);
int mcp7940n_write_sram(uint8_t offset, const uint8_t *data, uint8_t len);

int mcp7940n_mfp_alarm0_enable(void);
int mcp7940n_mfp_sqw_1hz(void);

#endif // MCP7940N_H