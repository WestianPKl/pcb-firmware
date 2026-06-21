#include "inputs.h"
#include "support.h"
#include "outputs.h"
#include "systick.h"


void init_btn(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN; // enable GPIOC clock
    (void)RCC->AHB1ENR; // delay after enabling clock

    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN; // enable SYSCFG clock
    (void)RCC->APB2ENR; // delay after enabling clock

    GPIOC->MODER &= ~(0x3U << (13U * 2U)); // clear bity [27:26]
    GPIOC->MODER |=  (0x0U << (13U * 2U)); // INPUT
    GPIOC->PUPDR &= ~(0x3U << (13U * 2U)); // no pull
    GPIOC->PUPDR |=  (0x2U << (13U * 2U)); // pull-down

    SYSCFG->EXTICR[3] &= ~(0xFU << (1U * 4U)); // clear EXTI13
    SYSCFG->EXTICR[3] |=  (0x2U << (1U * 4U)); // EXTI13 from PC13

    EXTI->PR = BTN_1; // clear pending bit
    EXTI->IMR &= ~BTN_1; // disable interrupt
    EXTI->FTSR &= ~BTN_1; // disable falling edge trigger

    EXTI->PR |= BTN_1; // clear pending bit
    EXTI->IMR |= BTN_1; // enable interrupt
    EXTI->FTSR |= BTN_1; // enable falling edge trigger

    NVIC_SetPriority(EXTI15_10_IRQn, 0); // set highest priority
    NVIC_EnableIRQ(EXTI15_10_IRQn); // enable EXTI15_10 interrupt   
}

void btn1_handler(void)
{
    systick_delay_ms(10);
    if (GPIOC->IDR & BTN_1) return;

    GPIOB->ODR ^= LED_1;
   
}