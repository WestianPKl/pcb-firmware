#include "led.h"
#include "output.h"

volatile uint8_t led1_state = 0U;
volatile uint8_t led2_state = 0U;
volatile uint8_t led3_state = 0U;

void led_init(void)
{
    gpio_output_init(GPIOB, LED1_PIN);
    gpio_output_init(GPIOB, LED2_PIN);
    gpio_output_init(GPIOB, LED3_PIN);
}

void led_on(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_on(GPIOx, pin);
    if (pin == LED1_PIN) led1_state = 1U;
    else if (pin == LED2_PIN) led2_state = 1U;
    else if (pin == LED3_PIN) led3_state = 1U;
}

void led_off(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_off(GPIOx, pin);
    if (pin == LED1_PIN) led1_state = 0U;
    else if (pin == LED2_PIN) led2_state = 0U;
    else if (pin == LED3_PIN) led3_state = 0U;
}

static uint8_t comm_led_ticks = 0U;

void comm_led_activity(void)
{
    led_on(GPIOB, LED3_PIN);
    comm_led_ticks = 5U;
}

void comm_led_process_100ms(void)
{
    if (comm_led_ticks > 0U)
    {
        comm_led_ticks--;

        if (comm_led_ticks == 0U)
        {
            led_off(GPIOB, LED3_PIN);
        }
    }
}