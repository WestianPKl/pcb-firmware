#include "outputs.h"
#include "support.h"

void init_leds(void)
{
    static const uint8_t leds[] = { LED1, LED3 };

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    for (uint8_t i = 0; i < sizeof(leds); i++) {
        uint8_t pin = leds[i];
        GPIOB->MODER &= ~(0x3U << (pin * 2U));
        GPIOB->MODER |=  (0x1U << (pin * 2U));
        GPIOB->PUPDR &= ~(0x3U << (pin * 2U));
        GPIOB->ODR   &= ~(1U << pin);
    }
}