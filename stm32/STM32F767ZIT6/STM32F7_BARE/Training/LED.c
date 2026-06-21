#include <stdint.h>

#define PERIPH_BASE 0x40000000UL
#define AHB1_OFFSET 0x20000UL
#define AHB1PERIPH_BASE PERIPH_BASE + AHB1_OFFSET
#define GPIOB_OFFSET 0x400UL
#define GPIOB_BASE AHB1PERIPH_BASE + GPIOB_OFFSET

#define RCC_OFFSET 0x3800UL
#define RCC_BASE AHB1PERIPH_BASE + RCC_OFFSET

#define RCC_AHB1ENR_OFFSET 0x30UL
#define RCC_AHB1ENR_BASE (*(volatile uint32_t *)(RCC_BASE + RCC_AHB1ENR_OFFSET))

#define GPIOB_CLK_ENABLE (1U << 1)
#define GPIOB_CLK_DISABLE (0U << 1)

#define GPIO_MODER_OFFSET 0x00UL
#define GPIO_OTYPER_OFFSET 0x04UL
#define GPIO_OSPEEDR_OFFSET 0x08UL
#define GPIO_PUPDR_OFFSET 0x0CUL
#define GPIO_ODR_OFFSET 0x14UL
#define GPIO_BSRR_OFFSET 0x18UL
#define GPIO_LCKR_OFFSET 0x1CUL
#define GPIO_AFRL_OFFSET 0x20UL
#define GPIO_AFRH_OFFSET 0x24UL

#define GPIOB_MODER_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_MODER_OFFSET))
#define GPIOB_OTYPER_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_OTYPER_OFFSET))
#define GPIOB_OSPEEDR_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_OSPEEDR_OFFSET))
#define GPIOB_PUPDR_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_PUPDR_OFFSET))
#define GPIOB_ODR_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_ODR_OFFSET))
#define GPIOB_BSRR_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_BSRR_OFFSET))
#define GPIOB_LCKR_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_LCKR_OFFSET))
#define GPIOB_AFRL_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_AFRL_OFFSET))
#define GPIOB_AFRH_BASE (*(volatile uint32_t *)(GPIOB_BASE + GPIO_AFRH_OFFSET))

// LED1 PB0
// LED2 PB7
// LED3 PB14
#define LED_1  (1U << 0)
#define LED_2  (1U << 7)
#define LED_3  (1U << 14)

void init_leds(void)
{
   RCC_AHB1ENR_BASE |= GPIOB_CLK_ENABLE;

    GPIOB_MODER_BASE &= ~(0x3U << (0 * 2));   // clear bity [1:0]
    GPIOB_MODER_BASE |=  (0x1U << (0 * 2));   // OUTPUT
    GPIOB_PUPDR_BASE &= ~(0x3U << (0 * 2));   // no pull
    GPIOB_ODR_BASE   &= ~(1U << 0);           // LED off

    GPIOB_MODER_BASE &= ~(0x3U << (7 * 2));   // clear bity [15:14]
    GPIOB_MODER_BASE |=  (0x1U << (7 * 2));   // OUTPUT
    GPIOB_PUPDR_BASE &= ~(0x3U << (7 * 2));   // no pull
    GPIOB_ODR_BASE   &= ~(1U << 7);           // LED off

    GPIOB_MODER_BASE &= ~(0x3U << (14 * 2));  // clear bity [29:28]
    GPIOB_MODER_BASE |=  (0x1U << (14 * 2));  // OUTPUT
    GPIOB_PUPDR_BASE &= ~(0x3U << (14 * 2));  // no pull
    GPIOB_ODR_BASE   &= ~(1U << 14);          // LED off
}

void delay(volatile uint32_t count)
{
    for(; count != 0; count--);
}

int main(void)
{
    init_leds();

	for(;;)
    {
        GPIOB_ODR_BASE |= LED_1;
        delay(1000000);
        GPIOB_ODR_BASE &= ~LED_1;
        delay(1000000);

        GPIOB_ODR_BASE |= LED_2;
        delay(1000000);
        GPIOB_ODR_BASE &= ~LED_2;
        delay(1000000);

        GPIOB_ODR_BASE |= LED_3;
        delay(1000000);    
        GPIOB_ODR_BASE &= ~LED_3;
        delay(1000000);
    };
}
