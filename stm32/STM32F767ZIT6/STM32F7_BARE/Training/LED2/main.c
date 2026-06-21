#include <stdint.h>
#include "structs.h"

// LED1 PB0
// LED2 PB7
// LED3 PB14
#define LED_1  (1U << 0)
#define LED_2  (1U << 7)
#define LED_3  (1U << 14)

void init_leds(void)
{
    RCC->AHB1ENR |= GPIOB_CLK_ENABLE;

    GPIOB->MODER &= ~(0x3U << (0U * 2U));   // clear bity [1:0]
    GPIOB->MODER |=  (0x1U << (0U * 2U));   // OUTPUT
    GPIOB->PUPDR &= ~(0x3U << (0U * 2U));   // no pull
    GPIOB->ODR   &= ~(1U << 0U);           // LED off

    GPIOB->MODER &= ~(0x3U << (7U * 2U));   // clear bity [15:14]
    GPIOB->MODER |=  (0x1U << (7U * 2U));   // OUTPUT
    GPIOB->PUPDR &= ~(0x3U << (7U * 2U));   // no pull
    GPIOB->ODR   &= ~(1U << 7U);           // LED off

    GPIOB->MODER &= ~(0x3U << (14U * 2U));  // clear bity [29:28]
    GPIOB->MODER |=  (0x1U << (14U * 2U));  // OUTPUT
    GPIOB->PUPDR &= ~(0x3U << (14U * 2U));  // no pull
    GPIOB->ODR   &= ~(1U << 14U);          // LED off
}

void delay()
{
    for(int i=0; i < 1000000; i++);
}

int main(void)
{
    init_leds();

	for(;;)
    {
        GPIOB->ODR |= LED_1;
        delay();
        GPIOB->ODR &= ~LED_1;
        delay();

        GPIOB->ODR |= LED_2;
        delay();
        GPIOB->ODR &= ~LED_2;
        delay();

        GPIOB->ODR |= LED_3;
        delay();    
        GPIOB->ODR &= ~LED_3;
        delay();
    };
}
