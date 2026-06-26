#include "stm32f7xx.h"
#include "systick.h"
#include <stdio.h>
#include "support.h"
#include "uart.h"
#include "dma.h"
#include "inputs.h"
#include "outputs.h"
#include "timers.h"
#include "adc.h"
// #include "ld2450.h"
#include "i2c.h"
#include "spi.h"
// #include "bme280.h"
// #include "sht40.h"
#include "can.h"

uint32_t tx_mailbox;

int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0);

    systick_init();
    init_leds();
    init_btn();
    init_timer_tim2_ch1();
    init_timer_tim4_ch2_pwm();

    dma2_init();
    adc1_ch4_continuous(1);
    adc1_dma2_config(1, adc_data_buffer, ADC_BUFFER_SIZE);
    adc1_start_conversion();

    dma1_init();
    uart3_rxtx_init();
    uart2_rxtx_init();
    uart3_dma1_rx_start(uart3_data, uart3_buffer_size);
    uart2_dma1_rx_start(uart2_data, uart2_buffer_size);

    spi1_init();
    spi1_dma_rx_init();
    spi1_dma_tx_init();

    spi3_init();
    spi3_dma_rx_init();
    spi3_dma_tx_init();

    i2c1_slave_config();
    i2c2_master_config();

    can_params_init(CAN_MODE_LOOPBACK);
    can_filter_accept_all();

    // bme280_init();

    static uint8_t duty = 0;
    static int8_t  dir  = 5;

    can_start();

	for(;;) {

        // ld2450_process_rx();

        if (btn_pressed) {
            btn1_handler();

            // uint8_t master_tx[4] = {0xA5, 0xA5, 0xA5, 0xA5};
            // uint8_t slave_tx[4]  = {0x3C, 0x3C, 0x3C, 0x3C};
            // uint8_t master_rx[4] = {0};
            // uint8_t slave_rx[4]  = {0};

            // spi1_dma_tx_done = spi1_dma_rx_done = 0;
            // spi3_dma_tx_done = spi3_dma_rx_done = 0;

            // spi3_dma_receive((uint32_t)slave_rx, 4);
            // spi3_dma_transfer((uint32_t)slave_tx, 4);
            // spi1_dma_receive((uint32_t)master_rx, 4);
            // spi1_dma_transfer((uint32_t)master_tx, 4);

            // uint32_t timeout = 1000000U;
            // while ((!spi1_dma_tx_done || !spi3_dma_rx_done || !spi1_dma_rx_done) && --timeout) { }

            // while (SPI1->SR & SPI_SR_BSY) { }

            // printf("Master->Slave (SPI3 got): %02X %02X %02X %02X\n",
            //        slave_rx[0], slave_rx[1], slave_rx[2], slave_rx[3]);
            // printf("Slave->Master (SPI1 got): %02X %02X %02X %02X\n",
            //        master_rx[0], master_rx[1], master_rx[2], master_rx[3]);

            // uint8_t tx[4] = {0x11, 0x22, 0x33, 0x44};
            // i2c1_rx_done = 0;
            // i2c2_master_write(I2C1_SLAVE_ADDRESS, tx, 4);
            // uint8_t i2c_ok = i2c2_master_wait();

            // uint32_t i2c_to = 100000U;
            // while (!i2c1_rx_done && --i2c_to) { }

            // printf("I2C master ack=%u, slave got: %02X %02X %02X %02X\n",
            //        i2c_ok, i2c1_rx_buffer[0], i2c1_rx_buffer[1], i2c1_rx_buffer[2], i2c1_rx_buffer[3]);
            btn_pressed = 0;
        }

        if (clock_tick_flag) {
            clock_tick_flag = 0;
            GPIOB->ODR ^= (1U << LED3);

            // int16_t  target1_x     = ld2450_coord(ld2450_data[4],  ld2450_data[5]);
            // int16_t  target1_y     = ld2450_coord(ld2450_data[6],  ld2450_data[7]);
            // int16_t  target1_speed = ld2450_coord(ld2450_data[8],  ld2450_data[9]);
            // uint16_t target1_dist  = (uint16_t)(ld2450_data[10] | (ld2450_data[11] << 8));

            // printf("Target 1 - X: %d, Y: %d, Speed: %d, Distance: %u\n",
            //         target1_x, target1_y, target1_speed, target1_dist);
            // uart3_write_dma((uint8_t *)"DATA\n", 5);

            printf("ADC Data: %d\n", adc_data_buffer[0]);

            can_tx_header_typedef tx_header;
            uint8_t tx_data[4] = {0x10, 0x20, 0x30, 0x40};

            tx_header.std_id = 0x10;
            tx_header.ext_id = 0;
            tx_header.ide = CAN_ID_STD;
            tx_header.rtr = 0;
            tx_header.dlc = 4;
            tx_header.transmit_global_time = 0;
    
            can_add_tx_message(&tx_header, tx_data, &tx_mailbox);

            printf("Data 0: %d\n", tx_data[0]);


            // bme280_trigger_forced();
            // systick_delay_ms(10);
            // bme280_read_data(&temp_x100, &hum_x100, &press_pa);

            // int32_t t_frac = temp_x100 % 100;
            // if (t_frac < 0) t_frac = -t_frac;

            // printf("BME280 - Temp: %ld.%02ld C, Hum: %lu.%02lu %%, Press: %lu Pa\n",
            //        (long)(temp_x100 / 100), (long)t_frac,
            //        (unsigned long)(hum_x100 / 100), (unsigned long)(hum_x100 % 100),
            //        (unsigned long)press_pa);

            // int16_t sht40_temp_x100;
            // uint16_t sht40_hum_x100;

            // sht40_data_read_int(&sht40_temp_x100, &sht40_hum_x100);

            // printf("SHT40 - Temp: %d.%02d C, Hum: %u.%02u %%\n",
            //        sht40_temp_x100 / 100, sht40_temp_x100 % 100,
            //        sht40_hum_x100 / 100, sht40_hum_x100 % 100);



            duty += dir;
            if (duty >= 100 || duty == 0) dir = -dir;
            set_duty_timer_tim4_ch2_pwm(duty);
        }

        if (can_rx_ready) {
            can_rx_ready = 0;
            printf("Receive 0: %d\n", can_rx_data[0]);
        }

    };
}
