#include "stm32f7xx.h"
#include "systick.h"
#include <stdio.h>
#include "support.h"
#include "uart.h"
#include "inputs.h"
#include "outputs.h"
#include "timers.h"
#include "adc.h"

volatile uint8_t btn_pressed = 0;
volatile uint32_t tick_10ms = 0;
volatile uint32_t tick_100ms = 0;
volatile uint8_t clock_tick_flag = 0;

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0); // disable buffering for stdout
    init_leds();
    init_btn();
    systick_init();
    init_timer_TIM2_CH1();
    uart3_rxtx_init();
    adc_init();

    uint8_t data;

	for(;;) {

        if (btn_pressed) {
            btn1_handler();
            btn_pressed = 0;
        }

        if (clock_tick_flag) {
            clock_tick_flag = 0;
            GPIOB->ODR ^= LED_2; // toggle LED2
            printf("ADC Value: %lu\n", adc_read()); // print ADC value
            printf("Hello, world!\n");
        }
    };
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & BTN_1) {
        EXTI->PR |= BTN_1; // clear pending bit
        btn_pressed = 1;
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; // clear update interrupt flag
        tick_10ms++;

        if (tick_10ms >= 100) {
            tick_10ms = 0;
            clock_tick_flag = 1; // set clock tick flag every 1 second (100 * 10ms)
        }

        if (tick_10ms % 10 == 0) { // every 100ms
            tick_100ms++;
        }

        if (tick_100ms >= 10) {
            tick_100ms = 0;
            // Perform actions every 1 second
        }
    }
}
