#include "inputs.h"

// ESP32 PC5 (active low in your getter)
// BTN1 = PB0, BTN2 = PB1
// MFP = PB5

void esp32_status_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    GPIOC->MODER &= ~(3U << (5U * 2U));
    GPIOC->PUPDR &= ~(3U << (5U * 2U));
}

uint8_t esp32_status_get(void)
{
    return (GPIOC->IDR & (1U << 5U)) ? 0U : 1U;
}

static void gpio_irq_init_pb(uint8_t pin, IRQn_Type irqn, uint8_t pupd)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(3U << (pin * 2U));
    GPIOB->PUPDR &= ~(3U << (pin * 2U));
    GPIOB->PUPDR |= ((uint32_t)(pupd & 0x03U) << (pin * 2U));

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    uint32_t idx = pin / 4U;
    uint32_t pos = (pin % 4U) * 4U;
    SYSCFG->EXTICR[idx] &= ~(0xFU << pos);
    SYSCFG->EXTICR[idx] |=  (0x1U << pos);

    EXTI->IMR  &= ~(1U << pin);
    EXTI->FTSR &= ~(1U << pin);
    EXTI->RTSR &= ~(1U << pin);

    EXTI->PR = (1U << pin);
    EXTI->IMR |= (1U << pin);
    EXTI->FTSR |= (1U << pin);

    NVIC_EnableIRQ(irqn);
}

void btn1_irq_init(void) { gpio_irq_init_pb(0U, EXTI0_IRQn, 1U); }
void btn2_irq_init(void) { gpio_irq_init_pb(1U, EXTI1_IRQn, 1U); }
void mfp_irq_init(void)  { gpio_irq_init_pb(5U, EXTI9_5_IRQn, 0U); } 