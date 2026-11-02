#include <stdint.h>
#include "stm32c0xx.h"
#include "led.h"
#include "output.h"
#include "timer.h"
#include "support.h"
#include "fault.h"
#include "version.h"
#include "dma.h"
#include "uart.h"
#include "adc.h"
#include "i2c.h"
#include "ina.h"
#include "protocol.h"

static protocol_request_t protocol_request;
static protocol_response_t protocol_response;
static uint8_t protocol_tx_buffer[PROTOCOL_RESPONSE_FRAME_MAX];

static reset_reason_t reset_reason = {0};

static void system_fault_process(void)
{
    if (adc_data_valid == 0U)
    {
        fault_set(FAULT_ADC);

        led_off(GPIOB, LED1_PIN);
        led_off(GPIOB, LED2_PIN);
        led_off(GPIOB, LED3_PIN);

        return;
    }

    fault_clear(FAULT_ADC);

    if (adc_vin < VIN_MIN_MV || adc_vin > VIN_MAX_MV)
    {
        fault_set(FAULT_VIN_LINE);
    }
    else {
        fault_clear(FAULT_VIN_LINE);
    }

    if (adc_v12 < V12_MIN_MV || adc_v12 > V12_MAX_MV)
    {
        fault_set(FAULT_12V_LINE);
        led_off(GPIOB, LED1_PIN);
    }
    else {
        fault_clear(FAULT_12V_LINE);
        led_on(GPIOB, LED1_PIN);
    }

    if (adc_v5 < V5_MIN_MV || adc_v5 > V5_MAX_MV)
    {
        fault_set(FAULT_5V_LINE);
        led_off(GPIOB, LED2_PIN);
    }
    else {
        fault_clear(FAULT_5V_LINE);
        led_on(GPIOB, LED2_PIN);
    }

    if (adc_v33 < V33_MIN_MV || adc_v33 > V33_MAX_MV)
    {
        fault_set(FAULT_3V3_LINE);
        led_off(GPIOB, LED3_PIN);
    }
    else {
        fault_clear(FAULT_3V3_LINE);
        led_on(GPIOB, LED3_PIN);
    }
}

int main(void)
{
    reset_reason_get(&reset_reason);
    protocol_service_init(&reset_reason);

    led_init();
    fault_pin_init();

    tim3_init();

    iwdg_init();

    dma1_init();
    uart1_init(115200);
    uart1_dma1_rx_start(uart1_data, uart1_buffer_size);

    adc1_init();
    adc1_dma_init();
    adc1_start();

    i2c1_init();

    if (board_info_valid() == 0U)
    {
        fault_set(FAULT_BOARD_INFO);
    }

    led_off(GPIOB, LED1_PIN);
    led_off(GPIOB, LED2_PIN);
    led_off(GPIOB, LED3_PIN);

    (void)ina_update();
    fault_set(FAULT_INA);
    fault_set(FAULT_ADC);

	for(;;)
    {
        if ((uart1_rx_ready != 0U) && (uart1_tx_busy == 0U))
        {
            uart1_rx_ready = 0U;

            if (protocol_parse_request(uart1_data, uart1_rx_length, &protocol_request) == PROTOCOL_STATUS_OK)
            {
                protocol_process_request(&protocol_request, &protocol_response);
                uint16_t tx_length = protocol_build_response(&protocol_response, protocol_tx_buffer);

                if (tx_length > 0U)
                {
                    uart1_write_dma(protocol_tx_buffer, tx_length);
                }
            }

            uart1_dma1_rx_start(uart1_data, uart1_buffer_size);
        }

        if (clock_tick_flag != 0U)
        {
            clock_tick_flag = 0U;

            iwdg_refresh();

            if (ina_update() != 0U)
            {
                fault_clear(FAULT_INA);
            }
            else
            {
                fault_set(FAULT_INA);
            }

            adc_update();
            system_fault_process();
        }
    };
}
