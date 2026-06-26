#include "outputs.h"
#include "support.h"

void init_leds(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN; // enable GPIOB clock
    (void)RCC->AHB1ENR; // delay after enabling clock

    GPIOB->MODER &= ~(0x3U << (0U * 2U));   // clear bity [1:0]
    GPIOB->MODER |=  (0x1U << (0U * 2U));   // OUTPUT
    GPIOB->PUPDR &= ~(0x3U << (0U * 2U));   // no pull
    GPIOB->ODR   &= ~(1U << 0U);           // LED off

    GPIOB->MODER &= ~(0x3U << (7U * 2U));   // clear bity [15:14]
    GPIOB->MODER |=  (0x1U << (7U * 2U));   // OUTPUT
    GPIOB->PUPDR &= ~(0x3U << (7U * 2U));   // no pull
    GPIOB->ODR   &= ~(1U << 7U);           // LED off

}