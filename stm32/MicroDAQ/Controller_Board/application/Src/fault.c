#include "fault.h"

static uint16_t fault_flags = 0U;

uint16_t fault_get(void)
{
    return fault_flags;
}

uint8_t fault_active(void)
{
    return (fault_flags != 0U) ? 1U : 0U;
}

void fault_set(uint16_t fault)
{
    fault_flags |= fault;
}

void fault_clear(uint16_t fault)
{
    fault_flags &= (uint16_t)~fault;
}