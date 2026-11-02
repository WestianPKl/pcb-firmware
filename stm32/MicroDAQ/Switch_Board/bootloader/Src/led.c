#include "led.h"
#include "output.h"

volatile uint8_t led1_state = 0U;
volatile uint8_t led2_state = 0U;
volatile uint8_t led3_state = 0U;
volatile uint8_t led4_state = 0U;


void led_init(void)
{
    gpio_output_init(GPIOA, LED1_PIN);
    gpio_output_init(GPIOA, LED2_PIN);
    gpio_output_init(GPIOA, LED3_PIN);
    gpio_output_init(GPIOA, LED4_PIN);
}

void led_on(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_on(GPIOx, pin);
}

void led_off(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    output_off(GPIOx, pin);
}