#include "digital_output.h"
#include "output.h"
#include <stddef.h>
#include "version.h"

volatile output_status_t output_status = {0};
volatile uint8_t output_max = 0U;

void digital_output_init(void)
{
    gpio_output_init(GPIOA, OUTPUT0_PIN);
    gpio_output_init(GPIOA, OUTPUT1_PIN);
    gpio_output_init(GPIOA, OUTPUT2_PIN);
    gpio_output_init(GPIOA, OUTPUT3_PIN);
    gpio_output_init(GPIOB, OUTPUT4_PIN);
    gpio_output_init(GPIOB, OUTPUT5_PIN);
    gpio_output_init(GPIOB, OUTPUT6_PIN);
    gpio_output_init(GPIOB, OUTPUT7_PIN);
    gpio_output_init(GPIOB, OUTPUT8_PIN);
    gpio_output_init(GPIOB, OUTPUT9_PIN);
    gpio_output_init(GPIOB, OUTPUT10_PIN);
    gpio_output_init(GPIOB, OUTPUT11_PIN);

    for (uint8_t i = 0U; i < OUTPUT_PHYSICAL_COUNT; i++)
    {
        digital_output_off(output_config[i].port, output_config[i].pin);
    }

    output_max = 0U;

    if (board_info_valid() != 0U)
    {
        const board_info_t *info = board_info_get();
        if ((info->capabilities.flags & CAP_DIGITAL_OUTPUT) != 0U)
        {
            output_max = info->capabilities.digital_output_count;
            if (output_max > OUTPUT_PHYSICAL_COUNT)
            {
                output_max = OUTPUT_PHYSICAL_COUNT;
            }
        }
    }
}

void digital_output_on(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_on(GPIOx, pin);
    if (GPIOx == GPIOA && pin == OUTPUT0_PIN) output_status.output0 = 1;
    if (GPIOx == GPIOA && pin == OUTPUT1_PIN) output_status.output1 = 1;
    if (GPIOx == GPIOA && pin == OUTPUT2_PIN) output_status.output2 = 1;
    if (GPIOx == GPIOA && pin == OUTPUT3_PIN) output_status.output3 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT4_PIN) output_status.output4 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT5_PIN) output_status.output5 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT6_PIN) output_status.output6 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT7_PIN) output_status.output7 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT8_PIN) output_status.output8 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT9_PIN) output_status.output9 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT10_PIN) output_status.output10 = 1;
    if (GPIOx == GPIOB && pin == OUTPUT11_PIN) output_status.output11 = 1;
}

void digital_output_off(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_off(GPIOx, pin);
    
    if (GPIOx == GPIOA && pin == OUTPUT0_PIN) output_status.output0 = 0;
    if (GPIOx == GPIOA && pin == OUTPUT1_PIN) output_status.output1 = 0;
    if (GPIOx == GPIOA && pin == OUTPUT2_PIN) output_status.output2 = 0;
    if (GPIOx == GPIOA && pin == OUTPUT3_PIN) output_status.output3 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT4_PIN) output_status.output4 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT5_PIN) output_status.output5 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT6_PIN) output_status.output6 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT7_PIN) output_status.output7 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT8_PIN) output_status.output8 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT9_PIN) output_status.output9 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT10_PIN) output_status.output10 = 0;
    if (GPIOx == GPIOB && pin == OUTPUT11_PIN) output_status.output11 = 0;
}

uint8_t digital_output_set_state(uint8_t output_number, uint8_t state)
{
    if ((output_number >= output_max) || (state > 1U))
    {
        return 0U;
    }

    if (state == 0U)
    {
        digital_output_off(output_config[output_number].port, output_config[output_number].pin);
    }
    else
    {
        digital_output_on(output_config[output_number].port, output_config[output_number].pin);
    }

    return 1U;
}

void digital_output_get_status(output_status_t *status)
{
    if (status != NULL)
    {
        *status = output_status;
    }
}

uint16_t digital_output_get_gpio_mask(void)
{
    uint16_t mask = 0U;
     if (GPIOA->IDR & (1U << OUTPUT0_PIN))  mask |= (1U << 0U);
     if (GPIOA->IDR & (1U << OUTPUT1_PIN))  mask |= (1U << 1U);
     if (GPIOA->IDR & (1U << OUTPUT2_PIN))  mask |= (1U << 2U);
     if (GPIOA->IDR & (1U << OUTPUT3_PIN))  mask |= (1U << 3U);
     if (GPIOB->IDR & (1U << OUTPUT4_PIN))  mask |= (1U << 4U);
     if (GPIOB->IDR & (1U << OUTPUT5_PIN))  mask |= (1U << 5U);
     if (GPIOB->IDR & (1U << OUTPUT6_PIN))  mask |= (1U << 6U);
     if (GPIOB->IDR & (1U << OUTPUT7_PIN))  mask |= (1U << 7U);
     if (GPIOB->IDR & (1U << OUTPUT8_PIN))  mask |= (1U << 8U);
     if (GPIOB->IDR & (1U << OUTPUT9_PIN))  mask |= (1U << 9U);
     if (GPIOB->IDR & (1U << OUTPUT10_PIN)) mask |= (1U << 10U);
     if (GPIOB->IDR & (1U << OUTPUT11_PIN)) mask |= (1U << 11U);

    return mask;
}

uint16_t digital_output_get_mask(void)
{
    uint16_t mask = 0U;
    mask |= ((uint16_t)output_status.output0  << 0U);
    mask |= ((uint16_t)output_status.output1  << 1U);
    mask |= ((uint16_t)output_status.output2  << 2U);
    mask |= ((uint16_t)output_status.output3  << 3U);
    mask |= ((uint16_t)output_status.output4  << 4U);
    mask |= ((uint16_t)output_status.output5  << 5U);
    mask |= ((uint16_t)output_status.output6  << 6U);
    mask |= ((uint16_t)output_status.output7  << 7U);
    mask |= ((uint16_t)output_status.output8  << 8U);
    mask |= ((uint16_t)output_status.output9  << 9U);
    mask |= ((uint16_t)output_status.output10 << 10U);
    mask |= ((uint16_t)output_status.output11 << 11U);
    
    return mask;
}

uint8_t digital_output_get_state(uint8_t output_number)
{
    if (output_number >= output_max)
    {
        return 0U;
    }
    return ((output_config[output_number].port->IDR & (1U << output_config[output_number].pin)) != 0U) ? 1U : 0U;
}