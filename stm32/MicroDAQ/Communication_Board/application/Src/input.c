#include "input.h"

volatile uint8_t slot_id = 0U;

void gpio_input_init(GPIO_TypeDef *GPIOx, uint32_t pin)
{
    if (GPIOx == GPIOA) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    } else if (GPIOx == GPIOB) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    } else if (GPIOx == GPIOC) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    } else if (GPIOx == GPIOD) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    } else if (GPIOx == GPIOE) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOEEN;
    }
    (void)RCC->AHB2ENR;

    GPIOx->MODER &= ~(0x3U << (pin * 2U));
    GPIOx->PUPDR &= ~(0x3U << (pin * 2U));
}

void input_init(void)
{
    gpio_input_init(GPIOB, SLOT_ID0_PIN);
    gpio_input_init(GPIOB, SLOT_ID1_PIN);
}

void slot_id_read(void)
{
    uint8_t slot_id0 = (GPIOB->IDR & (1U << SLOT_ID0_PIN)) ? 1U : 0U;
    uint8_t slot_id1 = (GPIOB->IDR & (1U << SLOT_ID1_PIN)) ? 1U : 0U;
    slot_id = ((slot_id1 << 1U) | slot_id0) + 1U;
}