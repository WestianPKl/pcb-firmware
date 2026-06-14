#include <stdint.h>
#include <string.h>
#include "stm32f4xx.h"
#include "systick.h"
#include "inputs.h"
#include "support.h"
#include "version.h"
#include "outputs.h"
#include "timer.h"
#include "adc.h"
#include "uart.h"
#include "dma.h"
#include "i2c.h"
#include "lcd.h"
#include "sht40.h"
#include "rtc.h"
#include "bme280.h"
#include "spi.h"
#include "rtc_locale.h"
#include "ina.h"
#include "mcp7940n.h"
#include "fm24cl16b.h"
#include "mx25l25673gm2i.h"
#include "uart_protocol.h"
#include "app_flags.h"
#include "flash_log.h"
#include "can.h"
#include "can_protocol.h"

void btn1_handler(void);
void btn2_handler(void);

volatile uint32_t tick_10ms = 0;
volatile uint8_t lcd_reinit_5s_flag = 0;
static uint16_t tick_10ms_5s = 0;
volatile uint16_t app_seconds_10min_cnt = 0;

volatile uint8_t esp32_on = 0;
volatile uint8_t esp32_toggle_flag = 0;
volatile uint16_t esp32_timer = 0;
volatile uint8_t lcd_refresh_flag = 0;
volatile uint8_t measure_flag_1s = 0;
volatile uint8_t can_100ms_flag = 0;
volatile uint8_t measure_flag_10min = 0;
volatile uint8_t second_marker = 0;

volatile uint8_t i2c1_dma_tx_done = 0;
volatile uint8_t i2c1_dma_rx_done = 0;
volatile uint8_t i2c1_dma_err     = 0;

volatile uint8_t spi1_dma_rx_done = 0;
volatile uint8_t spi1_dma_tx_done = 0;

static void app_second_tick(void)
{
    measure_flag_1s = 1;
    lcd_refresh_flag = 1;

    app_seconds_10min_cnt++;
    if (app_seconds_10min_cnt >= 600U) {
        app_seconds_10min_cnt = 0;
        measure_flag_10min = 1;
    }
}

static void clock_init_84mhz_from_hsi(void)
{
    RCC->CR |= RCC_CR_HSION;
    while ((RCC->CR & RCC_CR_HSIRDY) == 0U) {}

    RCC->CFGR &= ~RCC_CFGR_SW;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_HSI) {}

    RCC->CR &= ~RCC_CR_PLLON;
    while (RCC->CR & RCC_CR_PLLRDY) {}

    RCC->PLLCFGR =
        (16U << RCC_PLLCFGR_PLLM_Pos) |
        (168U << RCC_PLLCFGR_PLLN_Pos) |
        (0U << RCC_PLLCFGR_PLLP_Pos) |
        (4U << RCC_PLLCFGR_PLLQ_Pos) |
        RCC_PLLCFGR_PLLSRC_HSI;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1;
    RCC->CFGR |= RCC_CFGR_PPRE1_DIV2;
    RCC->CFGR |= RCC_CFGR_PPRE2_DIV1;

    FLASH->ACR =
        FLASH_ACR_ICEN |
        FLASH_ACR_DCEN |
        FLASH_ACR_PRFTEN |
        FLASH_ACR_LATENCY_2WS;

    RCC->CR |= RCC_CR_PLLON;
    while ((RCC->CR & RCC_CR_PLLRDY) == 0U) {}

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}
    SystemCoreClock = 84000000U;
}

int main(void)
{
    __disable_irq();

    clock_init_84mhz_from_hsi();

    systick_init();

    SCB->VTOR = 0x08008000U;
    __DSB(); __ISB();

    portc_init();
    portb_init();

    btn1_irq_init();
    btn2_irq_init();
    mfp_irq_init();

    timer13_init_10ms();

    timer3_pwm_ch1_init(84, 1000);
    timer3_pwm_ch2_init(84, 1000);
    timer3_pwm_ch3_init(84, 1000);
    timer8_pwm_ch4_init(84, 1000);

    timer1_pwm_ch1_init(84, 1000);
    timer2_pwm_ch3_init(84, 1000);
    timer4_pwm_ch3_init(84, 1000);
    timer4_pwm_ch4_init(84, 1000);

    dma1_init();
    dma2_init();

    uart1_rxtx_init();
    uart2_rxtx_init();

    dma1_uart2_rx_config(uart2_rx_buf, UART2_RX_BUFFER_SIZE);
    dma2_uart1_rx_config(uart1_rx_buf, UART1_RX_BUFFER_SIZE);

    i2c1_init();
    dma_i2c1_rx_init();
    dma_i2c1_tx_init();

    spi1_init();
    dma_spi1_rx_init();
    dma_spi1_tx_init();

    uint8_t cfg_flags = 0;
    if (fm24cl16b_read_byte(FRAM_ADDR_FLAGS, &cfg_flags) == 1) {
        ext_rtc_present = (cfg_flags & FRAM_FLAG_EXT_RTC_PRESENT) ? 1U : 0U;
        flash_present  = (cfg_flags & FRAM_FLAG_FLASH_PRESENT)   ? 1U : 0U;
        display_present = (cfg_flags & FRAM_FLAG_DISPLAY_PRESENT) ? 1U : 0U;
        sht40_present  = (cfg_flags & FRAM_FLAG_SHT40_PRESENT)   ? 1U : 0U;
        bme280_present = (cfg_flags & FRAM_FLAG_BME280_PRESENT)  ? 1U : 0U;
        ina226_present = (cfg_flags & FRAM_FLAG_INA226_PRESENT)  ? 1U : 0U;
        adc_present = (cfg_flags & FRAM_FLAG_ADC_PRESENT)      ? 1U : 0U;
        can_present = (cfg_flags & FRAM_FLAG_CAN_PRESENT)      ? 1U : 0U;
    } else {
        ext_rtc_present = 0U;
        flash_present   = 0U;
        display_present = 0U;
        sht40_present   = 0U;
        bme280_present  = 0U;
        ina226_present  = 0U;
        adc_present = 0U;
        can_present = 0U;
    }

    if (adc_present) {
        adc1_init(1, adc_data_buffer, ADC_BUFFER_SIZE);
        adc1_start_conversion();
        adc_was_present = 1;
    }

    if (display_present) {
        lcd_init();
        lcd_set_cursor(0, 0);
        lcd_send_string("Initializing...");
        backlight_on = 1;
        backlight_timer = 0;
        backlight_toggle_flag = 1;
    } else {
        if (lcd_is_present()) {
            lcd_clear();
            lcd_backlight(0);
        }
    }

    if (ext_rtc_present) {
        if (mcp7940n_init(1) == 1 && mcp7940n_mfp_sqw_1hz() == 1) {
        } else {
            ext_rtc_present = 0;
            rtc_init();
        }
    } else {
        rtc_init();
    }

    app_seconds_10min_cnt = 0;
    measure_flag_1s = 0;
    lcd_refresh_flag = 0;
    measure_flag_10min = 0;

    if (flash_present) {
        if (flash_log_init() != 1) {
            flash_present = 0U;
        }
    }

    if (ina226_present) {
        ina226_init(0x40, 500, 2000);
    }

    if (can_present) {
        can_gpio_init();
        can_params_init(CAN_MODE_NORMAL);
        can_filter_config(0x345);
        can_start();
    }

    __enable_irq();

    esp32_reset();

    if (bme280_present) {
        bme280_init();
    }

    while (1) {
        if (btn1_pressed) { btn1_pressed = 0; btn1_handler(); }
        if (btn2_pressed) { btn2_pressed = 0; btn2_handler(); }

        if (fram_flags_toggle_flag) {
            fram_flags_toggle_flag = 0;
            uint8_t cfg_flags_local = 0;
            if (fm24cl16b_read_byte(FRAM_ADDR_FLAGS, &cfg_flags_local) == 1) {
                ext_rtc_present = (cfg_flags_local & FRAM_FLAG_EXT_RTC_PRESENT) ? 1U : 0U;
                flash_present = (cfg_flags_local & FRAM_FLAG_FLASH_PRESENT)   ? 1U : 0U;
                display_present = (cfg_flags_local & FRAM_FLAG_DISPLAY_PRESENT) ? 1U : 0U;
                sht40_present  = (cfg_flags_local & FRAM_FLAG_SHT40_PRESENT)   ? 1U : 0U;
                bme280_present = (cfg_flags_local & FRAM_FLAG_BME280_PRESENT)  ? 1U : 0U;
                ina226_present = (cfg_flags_local & FRAM_FLAG_INA226_PRESENT)  ? 1U : 0U;
                adc_present = (cfg_flags_local & FRAM_FLAG_ADC_PRESENT)      ? 1U : 0U;
                can_present = (cfg_flags_local & FRAM_FLAG_CAN_PRESENT)      ? 1U : 0U;
            }

            if (adc_present && !adc_was_present) {
                adc1_init(1, adc_data_buffer, ADC_BUFFER_SIZE);
                adc1_start_conversion();
                adc_was_present = 1;
            } else if (!adc_present && adc_was_present) {
                adc1_stop_conversion();
                adc_was_present = 0;
            }

            if (display_present) {
                lcd_init();
                backlight_on = 0;
                backlight_timer = 0;
                backlight_toggle_flag = 1;
            } else {
                if (lcd_is_present()) 
                {
                    lcd_clear();
                    lcd_backlight(0);
                }
            }

            if (ext_rtc_present) {
                if (mcp7940n_init(1) == 1 && mcp7940n_mfp_sqw_1hz() == 1) {
                } else {
                    ext_rtc_present = 0;
                    rtc_init();
                }
            } else {
                rtc_init();
            }

            app_seconds_10min_cnt = 0;
            measure_flag_1s = 0;
            lcd_refresh_flag = 0;
            measure_flag_10min = 0;

            if (flash_present) {
                if (flash_log_init() != 1) {
                    flash_present = 0U;
                }
            }

            if (ina226_present) {
                ina226_init(0x40, 500, 2000);
            }

            if (can_present) {
                can_gpio_init();
                can_params_init(CAN_MODE_NORMAL);
                can_filter_config(0x345);
                can_start();
            }

            if (bme280_present) {
                bme280_init();
            }
        }

        uart2_process_rx();
        uart1_process_rx();

        if (can_100ms_flag) {
            can_100ms_flag = 0;
            send_cyclic_frames();
        }


        if (measure_flag_1s) {
            measure_flag_1s = 0;

            if (lcd_reinit_5s_flag && display_present) {
                lcd_reinit_5s_flag = 0;

                if (!lcd_is_present()) {
                    lcd_mark_present(1);
                    lcd_init();
                    if (lcd_is_present()) {
                        backlight_toggle_flag = 1;
                        backlight_on = 1;
                        lcd_clear();
                    }
                }
            }

            if (sht40_present) {
                int16_t  temp_c_x100 = 0;
                uint16_t rh_x100     = 0;

                uint8_t e = sht40_data_read_int(&temp_c_x100, &rh_x100);
                if (e != 0) {
                    measurement_sht40.temperature = 0;
                    measurement_sht40.humidity    = 0;
                    sht40_error_flag = 1;
                } else {
                    sht40_error_flag = 0;
                    measurement_sht40.temperature = temp_c_x100;
                    measurement_sht40.humidity    = rh_x100;
                }
            }

            if (ext_rtc_present) {
                mcp7940n_datetime_t dt_local;
                if (mcp7940n_get_datetime(&dt_local) == 1) {
                    datetime.year    = dt_local.year;
                    datetime.month   = dt_local.month;
                    datetime.day     = dt_local.mday;
                    datetime.weekday = dt_local.wday;
                    datetime.hours   = dt_local.hour;
                    datetime.minutes = dt_local.min;
                    datetime.seconds = dt_local.sec;
                }
            } else {
                rtc_read_datetime(&datetime.year, &datetime.month, &datetime.day, &datetime.weekday,
                                  &datetime.hours, &datetime.minutes, &datetime.seconds);
            }

            rtc_utc_to_warsaw(&datetime.year, &datetime.month, &datetime.day, &datetime.weekday,
                              &datetime.hours, &datetime.minutes, &datetime.seconds);

            if (bme280_present) {
                int32_t  temp_x100;
                uint32_t hum_x100;
                uint32_t press_pa;

                bme280_error_flag = 1;

                uint8_t id = bme280_read_id();
                if (id == 0x60) {
                    bme280_trigger_forced();
                    systick_delay_ms(10);

                    if (bme280_read_data(&temp_x100, &hum_x100, &press_pa) == 0) {
                        if (!(temp_x100 < -4000 || temp_x100 > 8500 || hum_x100 > 10000 || press_pa == 0)) {
                            measurement_bme280.temperature = temp_x100;
                            measurement_bme280.humidity    = hum_x100;
                            measurement_bme280.pressure    = press_pa;
                            bme280_error_flag = 0;
                        }
                    }
                }
            }
        }

        if (backlight_toggle_flag && display_present) {
            backlight_toggle_flag = 0;
            lcd_backlight(backlight_on);
        }

        if (lcd_refresh_flag && display_present && lcd_is_present()) {
            lcd_refresh_flag = 0;
            if (second_marker) second_marker = 0;
            else               second_marker = 1;

            uint16_t current_year = 2000 + datetime.year;

            if (current_year > 2000) {
                lcd_set_cursor(0, 0);
                lcd_send_decimal(current_year, 4);
                lcd_send_string("-");
                lcd_send_decimal(datetime.month, 2);
                lcd_send_string("-");
                lcd_send_decimal(datetime.day, 2);
                lcd_send_string(" ");
                lcd_send_decimal(datetime.hours, 2);
                if (second_marker) lcd_send_string(":");
                else               lcd_send_string(" ");
                lcd_send_decimal(datetime.minutes, 2);

                if (sht40_present) {
                    if (sht40_error_flag) {
                        lcd_set_cursor(0, 1);
                        lcd_send_string("SHT40 ERROR   ");
                    } else {
                        lcd_set_cursor(0, 1);
                        lcd_send_string("TH:");
                        lcd_send_string(" ");
                        lcd_send_temp_1dp_from_x100(measurement_sht40.temperature);
                        lcd_send_string(" ");
                        lcd_send_hum_1dp_from_x100(measurement_sht40.humidity);
                        lcd_send_string("  ");
                    }
                }

                if (bme280_present && !sht40_present) {
                    if (bme280_error_flag) {
                        lcd_set_cursor(0, 1);
                        lcd_send_string("BME280 ERROR  ");
                    } else {
                        lcd_set_cursor(0, 1);
                        lcd_send_string("TH:");
                        lcd_send_string(" ");
                        lcd_send_temp_1dp_from_x100((int16_t)measurement_bme280.temperature);
                        lcd_send_string(" ");
                        lcd_send_hum_1dp_from_x100((uint16_t)measurement_bme280.humidity);
                        lcd_send_string(" ");
                    }
                }
            } else {
                lcd_set_cursor(0, 0);
                lcd_send_string("Initializing... ");
                lcd_set_cursor(0, 1);
                lcd_send_string("                ");
            }
        }

        if (measure_flag_10min) {
            measure_flag_10min = 0;

            esp32_status_set(0);
            esp32_timer = 1;
            esp32_on = 1;

            if (flash_present) {
                uint32_t timestamp = 0;

                timestamp =
                    ((uint32_t)datetime.year   << 26) |
                    ((uint32_t)datetime.month  << 22) |
                    ((uint32_t)datetime.day    << 17) |
                    ((uint32_t)datetime.hours  << 12) |
                    ((uint32_t)datetime.minutes << 6) |
                    ((uint32_t)datetime.seconds);

                if (bme280_present && !bme280_error_flag) {
                    flash_log_append(timestamp, &measurement_bme280, 0);
                } else if (sht40_present && !sht40_error_flag) {
                    flash_log_append(timestamp, 0, &measurement_sht40);
                }
            }
        }

        if (can_rx_ready) {
            can_rx_ready = 0;
        }
    }
}

void btn1_handler(void)
{
    systick_delay_ms(10);
    if (GPIOB->IDR & (1U << 0U)) return;

    backlight_on = 1;
    backlight_timer = 0;
    backlight_toggle_flag = 1;
}

void btn2_handler(void)
{
    systick_delay_ms(10);
    if (GPIOB->IDR & (1U << 1U)) return;

    if (led2_state) { pin_set_low('B', 15U); led2_state = 0; }
    else            { pin_set_high('B', 15U); led2_state = 1; }
}

void EXTI0_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR0) { EXTI->PR = EXTI_PR_PR0; btn1_pressed = 1; }
}

void EXTI1_IRQHandler(void)
{
    if (EXTI->PR & EXTI_PR_PR1) { EXTI->PR = EXTI_PR_PR1; btn2_pressed = 1; }
}

void DMA2_Stream0_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF0) {
        DMA2->LIFCR = DMA_LIFCR_CTCIF0;
    }
}

void DMA1_Stream6_IRQHandler(void)
{
    if (DMA1->HISR & DMA_HISR_TCIF6) {
        DMA1->HIFCR = DMA_HIFCR_CTCIF6;
        uart2_tx_busy = 0;
    }
}

void DMA2_Stream7_IRQHandler(void)
{
    if (DMA2->HISR & DMA_HISR_TCIF7) {
        DMA2->HIFCR = DMA_HIFCR_CTCIF7;
        uart1_tx_busy = 0;
    }
}

void DMA1_Stream0_IRQHandler(void)
{
    if (DMA1->LISR & DMA_LISR_TEIF0) {
        DMA1->LIFCR = DMA_LIFCR_CTEIF0 | DMA_LIFCR_CTCIF0 | DMA_LIFCR_CHTIF0 | DMA_LIFCR_CDMEIF0 | DMA_LIFCR_CFEIF0;
        i2c1_dma_err = 1;
        i2c1_dma_rx_done = 1;
        return;
    }
    if (DMA1->LISR & DMA_LISR_TCIF0) {
        DMA1->LIFCR = DMA_LIFCR_CTCIF0;
        i2c1_dma_rx_done = 1;
    }
}

void DMA1_Stream1_IRQHandler(void)
{
    if (DMA1->LISR & DMA_LISR_TEIF1) {
        DMA1->LIFCR = DMA_LIFCR_CTEIF1 | DMA_LIFCR_CTCIF1 | DMA_LIFCR_CHTIF1 | DMA_LIFCR_CDMEIF1 | DMA_LIFCR_CFEIF1;
        i2c1_dma_err = 1;
        i2c1_dma_tx_done = 1;
        return;
    }
    if (DMA1->LISR & DMA_LISR_TCIF1) {
        DMA1->LIFCR = DMA_LIFCR_CTCIF1;
        i2c1_dma_tx_done = 1;
    }
}

void TIM8_UP_TIM13_IRQHandler(void)
{
    if (TIM13->SR & TIM_SR_UIF) {
        TIM13->SR &= ~TIM_SR_UIF;

        tick_10ms++;
        tick_10ms_5s++;

        if ((tick_10ms % 10U) == 0U) {
            can_100ms_flag = 1;
        }

        if (tick_10ms_5s >= 500U) {
            tick_10ms_5s = 0;
            lcd_reinit_5s_flag = 1;
        }

        if ((tick_10ms % 100U) == 0U) {
            if (!ext_rtc_present) {
                app_second_tick();
            }

            if (backlight_on && display_present) {
                backlight_timer++;
                if (backlight_timer >= 20U) {
                    backlight_on = 0;
                    backlight_timer = 0;
                    backlight_toggle_flag = 1;
                }
            }
        }

        if (esp32_timer > 0) {
            esp32_timer--;
            if (esp32_timer == 0) {
                if (esp32_on) {
                    esp32_status_set(1);
                    esp32_timer = 2;
                    esp32_on = 0;
                } else {
                    esp32_status_set(0);
                }
            }
        }
    }
}

void RTC_WKUP_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_WUTF) {
        rtc_write_protect_disable();
        RTC->ISR &= ~RTC_ISR_WUTF;
        rtc_write_protect_enable();

        rtc_exti_clear(20U);
        rtc_wakeup_flag = 1;
    }
}

void RTC_Alarm_IRQHandler(void)
{
    if (RTC->ISR & RTC_ISR_ALRAF) {
        rtc_write_protect_disable();
        RTC->ISR &= ~RTC_ISR_ALRAF;
        rtc_write_protect_enable();

        rtc_exti_clear(18U);
        rtc_alarm_flag = 1;
    }
}

void TAMP_STAMP_IRQHandler(void)
{
    rtc_exti_clear(19U);
    rtc_tampstamp_flag = 1;
}

void DMA2_Stream2_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF2) {
        spi1_dma_rx_done = 1;
        DMA2->LIFCR = DMA_LIFCR_CTCIF2;
    }
    if (DMA2->LISR & DMA_LISR_TEIF2) {
        DMA2->LIFCR = DMA_LIFCR_CTEIF2;
    }
}

void DMA2_Stream3_IRQHandler(void)
{
    if (DMA2->LISR & DMA_LISR_TCIF3) {
        spi1_dma_tx_done = 1;
        DMA2->LIFCR = DMA_LIFCR_CTCIF3;
    }
    if (DMA2->LISR & DMA_LISR_TEIF3) {
        DMA2->LIFCR = DMA_LIFCR_CTEIF3;
    }
}

void EXTI9_5_IRQHandler(void)
{
    if (EXTI->PR & (1U << 5)) {
        EXTI->PR = (1U << 5);

        if (ext_rtc_present) {
            app_second_tick();
        }
    }
}

void CAN1_RX0_IRQHandler(void)
{
    if (CAN1->RF0R & CAN_RF0R_FOVR0) {
        CAN1->RF0R = CAN_RF0R_FOVR0;
    }

    while (CAN1->RF0R & CAN_RF0R_FMP0) {
        can_rx_header_typedef hdr;
        uint8_t data[8];

        if (can_get_rx_message(CAN_RX_FIFO0, &hdr, data) == 0U) {
            can_rx_header_last = hdr;
            for (uint8_t i = 0; i < 8; i++) {
                can_rx_data_last[i] = data[i];
            }
            can_rx_ready = 1;
        } else {
            break;
        }
    }
}