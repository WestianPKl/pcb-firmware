#ifndef MCP23017_H
#define MCP23017_H

#include <stdint.h>
#include "stm32f7xx.h"

int mcp23017_init(void);
int mcp23017_set_pin_direction(uint8_t port, uint8_t pin, uint8_t direction);
int mcp23017_set_pin(uint8_t port, uint8_t pin, uint8_t value);

#endif // MCP23017_H