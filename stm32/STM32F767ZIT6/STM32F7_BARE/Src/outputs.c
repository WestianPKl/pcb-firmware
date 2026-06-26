#include "outputs.h"
#include "support.h"

void init_leds(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(0x3U << (LED1 * 2U));
    GPIOB->MODER |=  (0x1U << (LED1 * 2U));
    GPIOB->PUPDR &= ~(0x3U << (LED1 * 2U));
    GPIOB->ODR   &= ~(1U << LED1);

    GPIOB->MODER &= ~(0x3U << (LED3 * 2U));
    GPIOB->MODER |=  (0x1U << (LED3 * 2U));
    GPIOB->PUPDR &= ~(0x3U << (LED3 * 2U));
    GPIOB->ODR   &= ~(1U << LED3);

}