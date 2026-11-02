#include "fault.h"
#include "output.h"
#include "input.h"

static uint16_t fault_flags = 0U;

static void fault_update_pin(void)
{
    if (fault_flags != 0U)
    {
        output_on(GPIOB, FAULT_PIN);
    }
    else
    {
        output_off(GPIOB, FAULT_PIN);
    }
}

void fault_pin_init(void)
{
    gpio_output_init(GPIOB, FAULT_PIN);
    output_off(GPIOB, FAULT_PIN);
}

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
    fault_update_pin();
}

void fault_clear(uint16_t fault)
{
    fault_flags &= ~fault;
    fault_update_pin();
}