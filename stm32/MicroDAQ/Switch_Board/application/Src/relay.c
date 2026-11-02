#include "relay.h"
#include "output.h"
#include <stddef.h>
#include "version.h"

volatile relay_status_t relay_status = {0};
volatile uint8_t relay_max = 0U;

void relay_init(void)
{
    gpio_output_init(GPIOA, RELAY0_PIN);
    gpio_output_init(GPIOA, RELAY1_PIN);
    gpio_output_init(GPIOA, RELAY2_PIN);
    gpio_output_init(GPIOA, RELAY3_PIN);
    gpio_output_init(GPIOB, RELAY4_PIN);
    gpio_output_init(GPIOB, RELAY5_PIN);
    gpio_output_init(GPIOB, RELAY6_PIN);
    gpio_output_init(GPIOB, RELAY7_PIN);
    gpio_output_init(GPIOB, RELAY8_PIN);
    gpio_output_init(GPIOB, RELAY9_PIN);
    gpio_output_init(GPIOB, RELAY10_PIN);
    gpio_output_init(GPIOB, RELAY11_PIN);

    for (uint8_t i = 0U; i < RELAY_PHYSICAL_COUNT; i++)
    {
        relay_output_off(relay_config[i].port, relay_config[i].pin);
    }

    relay_max = 0U;

    if (board_info_valid() != 0U)
    {
        const board_info_t *info = board_info_get();
        if ((info->capabilities.flags & CAP_RELAY) != 0U)
        {
            relay_max = info->capabilities.relay_count;
            if (relay_max > RELAY_PHYSICAL_COUNT)
            {
                relay_max = RELAY_PHYSICAL_COUNT;
            }
        }
    }
}

void relay_output_on(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_on(GPIOx, pin);
    if (GPIOx == GPIOA && pin == RELAY0_PIN) relay_status.relay0 = 1;
    if (GPIOx == GPIOA && pin == RELAY1_PIN) relay_status.relay1 = 1;
    if (GPIOx == GPIOA && pin == RELAY2_PIN) relay_status.relay2 = 1;
    if (GPIOx == GPIOA && pin == RELAY3_PIN) relay_status.relay3 = 1;
    if (GPIOx == GPIOB && pin == RELAY4_PIN) relay_status.relay4 = 1;
    if (GPIOx == GPIOB && pin == RELAY5_PIN) relay_status.relay5 = 1;
    if (GPIOx == GPIOB && pin == RELAY6_PIN) relay_status.relay6 = 1;
    if (GPIOx == GPIOB && pin == RELAY7_PIN) relay_status.relay7 = 1;
    if (GPIOx == GPIOB && pin == RELAY8_PIN) relay_status.relay8 = 1;
    if (GPIOx == GPIOB && pin == RELAY9_PIN) relay_status.relay9 = 1;
    if (GPIOx == GPIOB && pin == RELAY10_PIN) relay_status.relay10 = 1;
    if (GPIOx == GPIOB && pin == RELAY11_PIN) relay_status.relay11 = 1;
}

void relay_output_off(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_off(GPIOx, pin);
    
    if (GPIOx == GPIOA && pin == RELAY0_PIN) relay_status.relay0 = 0;
    if (GPIOx == GPIOA && pin == RELAY1_PIN) relay_status.relay1 = 0;
    if (GPIOx == GPIOA && pin == RELAY2_PIN) relay_status.relay2 = 0;
    if (GPIOx == GPIOA && pin == RELAY3_PIN) relay_status.relay3 = 0;
    if (GPIOx == GPIOB && pin == RELAY4_PIN) relay_status.relay4 = 0;
    if (GPIOx == GPIOB && pin == RELAY5_PIN) relay_status.relay5 = 0;
    if (GPIOx == GPIOB && pin == RELAY6_PIN) relay_status.relay6 = 0;
    if (GPIOx == GPIOB && pin == RELAY7_PIN) relay_status.relay7 = 0;
    if (GPIOx == GPIOB && pin == RELAY8_PIN) relay_status.relay8 = 0;
    if (GPIOx == GPIOB && pin == RELAY9_PIN) relay_status.relay9 = 0;
    if (GPIOx == GPIOB && pin == RELAY10_PIN) relay_status.relay10 = 0;
    if (GPIOx == GPIOB && pin == RELAY11_PIN) relay_status.relay11 = 0;
}

uint8_t relay_set_state(uint8_t relay_number, uint8_t state)
{
    if ((relay_number >= relay_max) || (state > 1U))
    {
        return 0U;
    }

    if (state == 0U)
    {
        relay_output_off(relay_config[relay_number].port, relay_config[relay_number].pin);
    }
    else
    {
        relay_output_on(relay_config[relay_number].port, relay_config[relay_number].pin);
    }

    return 1U;
}

void relay_get_status(relay_status_t *status)
{
    if (status != NULL)
    {
        *status = relay_status;
    }
}

uint16_t relay_get_gpio_mask(void)
{
    uint16_t mask = 0U;
     if (GPIOA->IDR & (1U << RELAY0_PIN))  mask |= (1U << 0U);
     if (GPIOA->IDR & (1U << RELAY1_PIN))  mask |= (1U << 1U);
     if (GPIOA->IDR & (1U << RELAY2_PIN))  mask |= (1U << 2U);
     if (GPIOA->IDR & (1U << RELAY3_PIN))  mask |= (1U << 3U);
     if (GPIOB->IDR & (1U << RELAY4_PIN))  mask |= (1U << 4U);
     if (GPIOB->IDR & (1U << RELAY5_PIN))  mask |= (1U << 5U);
     if (GPIOB->IDR & (1U << RELAY6_PIN))  mask |= (1U << 6U);
     if (GPIOB->IDR & (1U << RELAY7_PIN))  mask |= (1U << 7U);
     if (GPIOB->IDR & (1U << RELAY8_PIN))  mask |= (1U << 8U);
     if (GPIOB->IDR & (1U << RELAY9_PIN))  mask |= (1U << 9U);
     if (GPIOB->IDR & (1U << RELAY10_PIN)) mask |= (1U << 10U);
     if (GPIOB->IDR & (1U << RELAY11_PIN)) mask |= (1U << 11U);

    return mask;
}

uint16_t relay_get_mask(void)
{
    uint16_t mask = 0U;
    mask |= ((uint16_t)relay_status.relay0  << 0U);
    mask |= ((uint16_t)relay_status.relay1  << 1U);
    mask |= ((uint16_t)relay_status.relay2  << 2U);
    mask |= ((uint16_t)relay_status.relay3  << 3U);
    mask |= ((uint16_t)relay_status.relay4  << 4U);
    mask |= ((uint16_t)relay_status.relay5  << 5U);
    mask |= ((uint16_t)relay_status.relay6  << 6U);
    mask |= ((uint16_t)relay_status.relay7  << 7U);
    mask |= ((uint16_t)relay_status.relay8  << 8U);
    mask |= ((uint16_t)relay_status.relay9  << 9U);
    mask |= ((uint16_t)relay_status.relay10 << 10U);
    mask |= ((uint16_t)relay_status.relay11 << 11U);
    
    return mask;
}

uint8_t relay_get_state(uint8_t relay_number)
{
    if (relay_number >= relay_max)
    {
        return 0U;
    }
    return ((relay_config[relay_number].port->IDR & (1U << relay_config[relay_number].pin)) != 0U) ? 1U : 0U;
}