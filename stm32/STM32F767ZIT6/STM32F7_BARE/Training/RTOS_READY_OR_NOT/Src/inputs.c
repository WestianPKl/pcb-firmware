#include "inputs.h"
#include "support.h"
#include "outputs.h"

volatile uint8_t btn_pressed = 0;

void init_btn(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;

    GPIOC->MODER &= ~(0x3U << (13U * 2U));
    GPIOC->MODER |=  (0x0U << (13U * 2U));
    GPIOC->PUPDR &= ~(0x3U << (13U * 2U));
    GPIOC->PUPDR |=  (0x2U << (13U * 2U));

    SYSCFG->EXTICR[3] &= ~(0xFU << (1U * 4U));
    SYSCFG->EXTICR[3] |=  (0x2U << (1U * 4U));

    EXTI->PR = BTN_1;
    EXTI->IMR &= ~BTN_1;
    EXTI->FTSR &= ~BTN_1;

    EXTI->PR |= BTN_1;
    EXTI->IMR |= BTN_1;
    EXTI->FTSR |= BTN_1;

    NVIC_SetPriority(EXTI15_10_IRQn, 0);
    NVIC_EnableIRQ(EXTI15_10_IRQn); 
}

void btn1_handler(void)
{
    systick_delay_ms(10);
    if (GPIOC->IDR & BTN_1) return;

    GPIOB->ODR ^= (1U << LED1);
   
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & BTN_1) {
        EXTI->PR |= BTN_1;
        btn_pressed = 1;
    }
}
