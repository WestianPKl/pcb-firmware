#include "input.h"

volatile uint8_t drv8876_fault_flag = 0;
volatile uint32_t drv8876_fault_counter = 0U;

void gpio_input_init(GPIO_TypeDef *GPIOx, uint32_t pin)
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
    GPIOx->PUPDR &= ~(0x3U << (pin * 2U));

    if (DRV8876_FAULT_Pin == pin && GPIOx == GPIOC) {
        RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
        (void)RCC->APB2ENR;

        EXTI->IMR &= ~(1U << DRV8876_FAULT_Pin);

        SYSCFG->EXTICR[2] &= ~SYSCFG_EXTICR3_EXTI8_Msk;
        SYSCFG->EXTICR[2] |= (2U << SYSCFG_EXTICR3_EXTI8_Pos);

        EXTI->RTSR &= ~(1U << DRV8876_FAULT_Pin);
        EXTI->FTSR |=  (1U << DRV8876_FAULT_Pin);

        EXTI->PR = (1U << DRV8876_FAULT_Pin);

        NVIC_ClearPendingIRQ(EXTI9_5_IRQn);
        NVIC_SetPriority(EXTI9_5_IRQn, 2U);
        NVIC_EnableIRQ(EXTI9_5_IRQn);

        EXTI->IMR |= (1U << DRV8876_FAULT_Pin);
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & (1U << DRV8876_FAULT_Pin)) {
        EXTI->PR = (1U << DRV8876_FAULT_Pin);
        drv8876_fault_counter++;
        drv8876_fault_flag = 1U;
    }
}