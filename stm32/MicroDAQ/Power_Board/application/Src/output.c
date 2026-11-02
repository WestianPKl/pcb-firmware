#include "output.h"

void gpio_output_init(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    if (GPIOx == GPIOA) {
        RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    } else if (GPIOx == GPIOB) {
        RCC->IOPENR |= RCC_IOPENR_GPIOBEN;
    } else if (GPIOx == GPIOC) {
        RCC->IOPENR |= RCC_IOPENR_GPIOCEN;
    } else if (GPIOx == GPIOD) {
        RCC->IOPENR |= RCC_IOPENR_GPIODEN;
    } else if (GPIOx == GPIOF) {
        RCC->IOPENR |= RCC_IOPENR_GPIOFEN;
    }
    (void)RCC->IOPENR;

    GPIOx->MODER &= ~(0x3U << (pin * 2U));
    GPIOx->MODER |= (0x1U << (pin * 2U));
    GPIOx->OTYPER &= ~(0x1U << pin);
    GPIOx->OSPEEDR &= ~(0x3U << (pin * 2U));
    GPIOx->PUPDR &= ~(0x3U << (pin * 2U));
}

void output_on(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    GPIOx->BSRR = (0x1U << pin);
}

void output_off(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    GPIOx->BSRR = (0x1U << (pin + 16U));
}