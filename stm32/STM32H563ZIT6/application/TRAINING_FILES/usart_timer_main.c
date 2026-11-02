#include <stdint.h>
#include "stm32h5xx.h"
#include "support.h"
#include "output.h"
#include "input.h"
#include "timer.h"
#include "uart.h"

uint8_t pwm_state = 0;

int main(void)
{
    clock_init_250mhz();

    gpio_output_init(GPIOB, GREEN_LED_PIN);
    gpio_output_init(GPIOF, YELLOW_LED_PIN);
    gpio_output_init(GPIOG, RED_LED_PIN);
    gpio_input_init(GPIOC, BTN_PIN);
    tim2_init();
    tim3_ch1_pwm_init();
    tim3_ch1_pwm_set_frequency(2000U);
    tim3_ch1_pwm_set_duty(25U);
    // tim3_ch1_compare_match_init();
    // tim4_input_capture_init();
    tim4_pwm_input_init();
    uart3_init();
    uart3_tx_dma_init();
    uart3_rx_dma_init();

    uart3_dma_read(
        uart3_dma_rx_buffer,
        UART3_RX_BUFFER_SIZE
    );

	for(;;)
    {   

        if (uart3_dma_rx_done)
        {
            uart3_dma_rx_done = 0U;
        }

        if (uart3_idle_detected)
        {
            uart3_idle_detected = 0U;

            if (uart3_rx_current_pos > uart3_rx_old_pos)
            {
                uart3_rx_process(
                    (const uint8_t *)&uart3_dma_rx_buffer[uart3_rx_old_pos],
                    uart3_rx_current_pos - uart3_rx_old_pos
                );
            }
            else if (uart3_rx_current_pos < uart3_rx_old_pos)
            {
                uart3_rx_process(
                    (const uint8_t *)&uart3_dma_rx_buffer[uart3_rx_old_pos],
                    UART3_RX_BUFFER_SIZE - uart3_rx_old_pos
                );

                if (uart3_rx_current_pos > 0U)
                {
                    uart3_rx_process(
                        (const uint8_t *)&uart3_dma_rx_buffer[0],
                        uart3_rx_current_pos
                    );
                }
            }
            uart3_rx_old_pos = uart3_rx_current_pos;
        }

        if (clock_tick_flag)
        {
            clock_tick_flag = 0;
            uart3_dma_write(
                (const uint8_t *)"Hello DMA\r\n",
                11U
            );
            pwm_state++;
            if (pwm_state == 1){
                tim3_ch1_pwm_set_duty(0U);
                // tim3_ch1_pwm_set_frequency(0U);
            } else if (pwm_state == 2){
                tim3_ch1_pwm_set_duty(25U);
                // tim3_ch1_pwm_set_frequency(1000U);
            } else if (pwm_state == 3){
                tim3_ch1_pwm_set_duty(50U);
                // tim3_ch1_pwm_set_frequency(2000U);
            } else {
                tim3_ch1_pwm_set_duty(100U);
                // tim3_ch1_pwm_set_frequency(4000U);
                pwm_state = 0;
            }

            pwm_measurement_t pwm_measurement_local;
            tim4_pwm_get_measurement(&pwm_measurement_local);

            if (yellow_led_state)
            {
                output_off(GPIOF, YELLOW_LED_PIN);
                yellow_led_state = 0;
            }
            else
            {
                output_on(GPIOF, YELLOW_LED_PIN);
                yellow_led_state = 1;
            }
        }

        if (compare_1s_flag)
        {
            compare_1s_flag = 0;
            if (red_led_state){
                output_off(GPIOG, RED_LED_PIN);
                red_led_state = 0;
            }
            else{
                output_on(GPIOG, RED_LED_PIN);
                red_led_state = 1;
            }
            
        }

        if (btn1_debounce_done)
        {
            btn1_debounce_done = 0;
            btn1_changed_callback();
            if (btn1_stable_state == 1)
            {
                output_on(GPIOB, GREEN_LED_PIN);
            }
            else
            {
                output_off(GPIOB, GREEN_LED_PIN);
            }
        }
    }
}