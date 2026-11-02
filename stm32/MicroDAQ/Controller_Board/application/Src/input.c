#include "input.h"

volatile uint8_t btn1_debounce_ticks = 0U;
volatile uint8_t btn1_debounce_active = 0U;
volatile uint8_t btn1_debounce_done = 0U;
volatile uint8_t btn1_stable_state = 0U;

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
    } else if (GPIOx == GPIOF) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOFEN;
    } else if (GPIOx == GPIOG) {
        RCC->AHB2ENR |= RCC_AHB2ENR_GPIOGEN;
    }
    (void)RCC->AHB2ENR;

    GPIOx->MODER &= ~(0x3U << (pin * 2U));
    GPIOx->PUPDR &= ~(0x3U << (pin * 2U));

    if (((GPIOx == GPIOC) && (pin == 11U)) || ((GPIOx == GPIOE) && (pin == 5U))  || ((GPIOx == GPIOE) && (pin == 8U)))
    {
        GPIOx->PUPDR |= (0x1U << (pin * 2U));
    }
    else
    {
        GPIOx->PUPDR |= (0x2U << (pin * 2U));
    }

    if (pin == BTN_PIN && GPIOx == GPIOC) {
        EXTI->IMR1 &= ~(0x1U << BTN_PIN);

        EXTI->EXTICR[3] &= ~(0xFF << 8U);
        EXTI->EXTICR[3] |= (0x2U << 8U);

        EXTI->RTSR1 &= ~(0x1U << BTN_PIN);
        EXTI->FTSR1 &= ~(0x1U << BTN_PIN);

        EXTI->RTSR1 |= (0x1U << BTN_PIN);
        EXTI->FTSR1 |= (0x1U << BTN_PIN);
        EXTI->RPR1 = (0x1U << BTN_PIN);
        EXTI->FPR1 = (0x1U << BTN_PIN);

        NVIC->IPR[24] = 0x50U;
        NVIC->ICPR[0] = (1UL << 24U);
        NVIC->ISER[0] = (1UL << 24U);

        EXTI->IMR1 |= (0x1U << BTN_PIN);
    }
}

void btn1_changed_callback(void)
{
    uint8_t new_state = (GPIOC->IDR & (0x1U << BTN_PIN)) == 0U ? 0U : 1U;
    if (new_state != btn1_stable_state){
        btn1_stable_state = new_state;
    }
}

void EXTI13_IRQHandler(void)
{
    if ((EXTI->RPR1 & (1U << BTN_PIN)) ||
        (EXTI->FPR1 & (1U << BTN_PIN)))
    {
        EXTI->RPR1 = (0x1U << BTN_PIN);
        EXTI->FPR1 = (0x1U << BTN_PIN);
        btn1_debounce_ticks = 3U;
        btn1_debounce_active = 1U;
    }
}