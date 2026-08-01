#include "mcp23017.h"
#include "i2c.h"

#define MCP23017_ADDRESS 0x20

int mcp23017_init(void)
{
    uint8_t zero = 0x00;
    int status = 1;

    if (i2c2_write_reg(MCP23017_ADDRESS, 0x00, &zero, 1) != 1U) {
        status = -1;
    }
    if (i2c2_write_reg(MCP23017_ADDRESS, 0x01, &zero, 1) != 1U) {
        status = -1;
    }

    return status;
}

int mcp23017_set_pin_direction(uint8_t port, uint8_t pin, uint8_t direction)
{
    if (port > 1 || pin > 7) {
        return -1;
    }

    uint8_t reg = (port == 0) ? 0x00 : 0x01;
    uint8_t current_direction = 0;

    if (i2c2_read_reg(MCP23017_ADDRESS, reg, &current_direction, 1) != 1U) {
        return -1;
    }

    if (direction) {
        current_direction |= (1U << pin);
    } else {
        current_direction &= ~(1U << pin);
    }

    return (i2c2_write_reg(MCP23017_ADDRESS, reg, &current_direction, 1) == 1U) ? 1 : -1;
}

int mcp23017_set_pin(uint8_t port, uint8_t pin, uint8_t value)
{
    if (port > 1 || pin > 7) {
        return -1;
    }

    uint8_t reg = (port == 0) ? 0x14 : 0x15;
    uint8_t current_value = 0;

    if (i2c2_read_reg(MCP23017_ADDRESS, reg, &current_value, 1) != 1U) {
        return -1;
    }

    if (value) {
        current_value |= (1U << pin);
    } else {
        current_value &= ~(1U << pin);
    }

    return (i2c2_write_reg(MCP23017_ADDRESS, reg, &current_value, 1) == 1U) ? 1 : -1;
}

