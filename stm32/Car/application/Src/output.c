#include "output.h"

void gpio_output_init(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    if (GPIOx == GPIOA) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    } else if (GPIOx == GPIOB) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    } else if (GPIOx == GPIOC) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    } else if (GPIOx == GPIOD) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    } else if (GPIOx == GPIOE) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOEEN;
    } else if (GPIOx == GPIOF) {
        RCC->AHB1ENR |= RCC_AHB1ENR_GPIOFEN;
    } else if (GPIOx == GPIOG) {
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOGEN;
    }
    (void)RCC->AHB1ENR;

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