#include <stdint.h>
#include "stm32h5xx.h"
#include "support.h"
#include "output.h"
#include "input.h"
#include "timer.h"
#include "uart.h"
#include "i2c.h"
#include "protocol.h"
#include "tca9548.h"
#include "sht40.h"
#include "bme280.h"
#include "mcp23017.h"
#include "spi.h"
#include "adc.h"
#include "ai.h"
// #include "nrf.h"
#include "can.h"
#include "ethernet.h"
#include "ld2450.h"
#include "card_manager.h"

#include "ethernet.h"
#include "ethernetif.h"
// #include "udp_echo.h"
// #include "tcp_echo.h"
#include "controller_tcp.h"

#include "lwip/init.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/timeouts.h"
#include "netif/ethernet.h"

#include "systick.h"
#include "rtc.h"
#include "fmac.h"


static struct netif gnetif;

uint8_t pwm_state = 0U;
uint8_t pin_state = 0U;
sht40_data_t sht40_result = {0};
bme280_data_t bme280_result = {0};
uint16_t adc_value;
uint8_t ai_class = 0U;
uint8_t ai_status = 0U;
volatile uint8_t ethernet_state = 0U;
// uint8_t data[NRF24_PAYLOAD_SIZE] =
// {
//     0x11,
//     0x22,
//     0x33,
//     0x44,
//     0x55,
//     0x66,
//     0x77,
//     0x88
// };
// uint8_t rx_data[NRF24_PAYLOAD_SIZE];
uint32_t can_id;
can_id_type_t id_type;
uint8_t can_data[64];
uint8_t can_length;
can_id_type_t can_id_type;
can_state_t state;

uint8_t *eth_frame;
uint16_t eth_len;

uint8_t tx[] = {0xAA, 0x55, 0x12, 0x34};
uint8_t rx[4] = {0};
uint8_t data_can[8] = {0x11, 0x22, 0x33, 0x44, 0x55, 0x66, 0x77, 0x00};
uint8_t data_fd[12] ={0x11, 0x22, 0x33, 0x44,0x55, 0x66, 0x77, 0x88,0x99, 0xAA, 0xBB, 0xCC};

rtc_datetime_t dt =
{
    .seconds = 0,
    .minutes = 45,
    .hours = 16,
    .weekday = 6,
    .day = 12,
    .month = 9,
    .year = 2026
};
rtc_datetime_t now;

volatile uint32_t random1;
volatile uint32_t random2;
volatile uint32_t random3;

volatile uint32_t crc_result;

volatile uint8_t dts_read_state;
volatile int32_t cpu_temperature = 0;

volatile int32_t cordic_cos;
volatile int32_t cordic_sin;
volatile uint8_t cordic_state;

volatile uint8_t fmac_state;
volatile int16_t fmac_y1;
volatile int16_t fmac_y2;
volatile int16_t fmac_y3;
volatile int16_t fmac_y4;

volatile uint32_t wakeup_counter = 0U;

uint8_t data_protocol_test[3] =
{
    0x00,   // COMMAND - Protocol Test
    0x01,   // SEQUENCE
    0x00,   // LENGTH
};

uint8_t data_identify[3] =
{
    0x01,   // COMMAND - Identify
    0x01,   // SEQUENCE
    0x00,   // LENGTH
};


uint8_t data_board[3] =
{
    0x02,   // COMMAND - Board
    0x01,   // SEQUENCE
    0x00,   // LENGTH
};

volatile uint8_t relay_state = 0U;

uint8_t set_relay1_on[5] =
{
    0x11,   // COMMAND - Identify
    0x05,   // SEQUENCE
    0x02,   // LENGTH
    0x01,   // RELAY NUMBER
    0x01    // RELAY STATE
};

uint8_t set_relay1_off[5] =
{
    0x11,   // COMMAND - Identify
    0x05,   // SEQUENCE
    0x02,   // LENGTH
    0x01,   // RELAY NUMBER
    0x00    // RELAY STATE
};

uint8_t set_relay2_on[5] =
{
    0x11,   // COMMAND - Identify
    0x05,   // SEQUENCE
    0x02,   // LENGTH
    0x02,   // RELAY NUMBER
    0x01    // RELAY STATE
};

uint8_t set_relay2_off[5] =
{
    0x11,   // COMMAND - Identify
    0x05,   // SEQUENCE
    0x02,   // LENGTH
    0x02,   // RELAY NUMBER
    0x00    // RELAY STATE
};

static uint16_t can_id_make(uint8_t slot, uint8_t offset)
{
    return ((uint16_t)slot << 8U) | offset;
}

int main(void)
{
    clock_init_250mhz();
    // reset_reason_read();
    // uint32_t reset_reason = reset_reason_get();
    // rtc_clock_init();


    // rtc_set_datetime(&dt);
    // rtc_backup_write(0xA55AA55AU);
    // rtc_alarm_init(10);
    // rtc_wakeup_init(5);
    // iwdg_init();
    // wwdg_init();



    // if (rng_init())
    // {
    //     rng_get((uint32_t *)&random1);
    //     rng_get((uint32_t *)&random2);
    //     rng_get((uint32_t *)&random3);
    // }
    systick_init(1000U);

    // uint32_t test_data[] =
    // {
    //     0x12345678U,
    //     0x9ABCDEF0U,
    //     0x11223344U
    // };

    // crc_init();

    // crc_result = crc32_calculate(
    //     test_data,
    //     sizeof(test_data) / sizeof(test_data[0]));

    // vrefbuf_init();

    gpio_output_init(GPIOB, GREEN_LED_PIN);
    gpio_output_init(GPIOF, YELLOW_LED_PIN);
    gpio_output_init(GPIOG, RED_LED_PIN);
    gpio_input_init(GPIOC, BTN_PIN);
    card_manager_init();
    tim2_init();
    tim6_init();
    tim3_ch1_pwm_init();
    tim3_ch1_pwm_set_frequency(2000U);
    tim3_ch1_pwm_set_duty(25U);
    tim4_pwm_input_init();

    // dts_init(); 
    // dts_read_state = dts_get_temperature((int32_t *)&cpu_temperature);

    // cordic_init();
    // cordic_state =
    //     cordic_cos_sin(0x20000000,
    //                 (int32_t *)&cordic_cos,
    //                 (int32_t *)&cordic_sin);

    // fmac_state = fmac_init();
    // lptim_state = lptim1_init_1s();

    // if (fmac_state != 0U)
    // {
    //     fmac_process_sample(1000, (int16_t *)&fmac_y1);
    //     fmac_process_sample(2000, (int16_t *)&fmac_y2);
    //     fmac_process_sample(3000, (int16_t *)&fmac_y3);
    //     fmac_process_sample(4000, (int16_t *)&fmac_y4);
    // }
    ethernet_state = ethernet_init();

    if (ethernet_state != 0U)
    {
        lwip_init();

        ip4_addr_t ipaddr;
        ip4_addr_t netmask;
        ip4_addr_t gateway;

        IP4_ADDR(&ipaddr,  192, 168, 18, 200);
        IP4_ADDR(&netmask, 255, 255, 255, 0);
        IP4_ADDR(&gateway, 192, 168, 18, 1);

        if (netif_add(&gnetif,
            &ipaddr,
            &netmask,
            &gateway,
            NULL,
            ethernetif_init,
            ethernet_input) != NULL)
        {
            netif_set_default(&gnetif);
            netif_set_up(&gnetif);
            netif_set_link_up(&gnetif);
        }

        // udp_echo_init();
        // tcp_echo_init();
        controller_tcp_init();
    }

    // uart2_init(115200U);
    // i2c1_init();
    // i2c2_init();

    // spi1_init(1U);
    // spi2_init();

    // adc1_init();
    // adc1_trigg_init();
    // adc1_dma_init();
    // adc1_dma_start();

    // bme280_init();
    // mcp23017_init();
    // mcp23017_set_pin_direction(0, 0, 0);

    // ai_status = ai_init();

    // if (ai_status != 0U)
    // {
    //     ai_run(0.10f, &ai_class);
    //     ai_run(0.40f, &ai_class);
    //     ai_run(0.50f, &ai_class);
    //     ai_run(0.80f, &ai_class);
    // }

    // nrf_init();
    can_init();

    // can_send_std(0x100U, data_protocol_test, sizeof(data_protocol_test));

	for(;;)
    {   
        // SCB->SCR &= ~SCB_SCR_SLEEPDEEP_Msk;
        // __WFI();
        // pwr_enter_stop();
        // wakeup_counter++;
        // iwdg_refresh();
        // wwdg_refresh_if_window();
        if (ethernet_state != 0U)
        {
            ethernetif_input(&gnetif);

            sys_check_timeouts();
        }

        if (can_rx0_irq != 0U)
        {
            can_rx0_irq = 0U;
            while (can_receive_fifo0(&can_id, &can_id_type, can_data, &can_length) != 0U)
            {
                if (can_id_type == CAN_ID_STD)
                {
                    card_manager_can_receive(can_id, can_data, can_length);
                }
            }
        }

        if (can_rx1_irq != 0U)
        {
            can_rx1_irq = 0U;
            while (can_receive_fifo1(&can_id, &can_id_type, can_data, &can_length) != 0U)
            {
                if (can_id_type == CAN_ID_STD)
                {
                    card_manager_can_receive(can_id, can_data, can_length);
                }
            }
        }

        card_manager_process();

        // if ((can_ew_irq != 0U) || (can_ep_irq != 0U) || (can_bo_irq != 0U))
        // {
        //     can_ew_irq = 0U;
        //     can_ep_irq = 0U;
        //     can_bo_irq = 0U;
        //     state = can_get_state();
        // }

        // if (can_tx_irq != 0U)
        // {
        //     can_tx_irq = 0U;
        // }
        // if (nrf_irq != 0U)
        // {
        //     nrf_irq = 0U;
        //     uint8_t event = nrf_process_irq(rx_data);
        //     if ((event & NRF_EVENT_RX_DATA) != 0U){}
        //     if ((event & NRF_EVENT_TX_OK) != 0U){}
        //     if ((event & NRF_EVENT_TX_FAILED) != 0U){}
        // }

        // spi2_process();
        // uart2_process();

        if (clock_tick_flag)
        {
            clock_tick_flag = 0U;

            card_manager_1s_process();

            if (relay_state != 0U)
            {
                for (uint8_t i = 0U; i < SLOT_COUNT; i++)
                {
                    if ((card_slots[i].identified != 0U) && (card_slots[i].service.online != 0U) && (card_slots[i].board_info.card_type == CARD_TYPE_SWITCH))
                    {
                        uint16_t request_id = can_id_make((uint8_t)(i + 1U), 0x00U);

                        can_send_std(request_id, set_relay1_on, sizeof(set_relay1_on));
                        can_send_std(request_id, set_relay2_on, sizeof(set_relay2_on));
                    }
                }

                relay_state = 0U;
            }
            else
            {
                for (uint8_t i = 0U; i < SLOT_COUNT; i++)
                {
                   if ((card_slots[i].identified != 0U) && (card_slots[i].service.online != 0U) && (card_slots[i].board_info.card_type == CARD_TYPE_SWITCH))
                    {
                        uint16_t request_id = can_id_make((uint8_t)(i + 1U), 0x00U);

                        can_send_std(request_id, set_relay1_off, sizeof(set_relay1_off));
                        can_send_std(request_id, set_relay2_off, sizeof(set_relay2_off));
                    }
                }

                relay_state = 1U;
            }

            // rtc_get_datetime(&now);
            // uint32_t backup_value = rtc_backup_read();
            // if (backup_value != 0xA55AA55AU)
            // {
            //     rtc_backup_write(0xA55AA55AU);
            // }
            // uint8_t tx_ok = ethernet_send_test_frame();

            // data_can[7]++;
            // can_send(0x100U, data_can, 8U);
            // can_send_std(0x100U, data_fd, 12U);
            // if (data_can[7] == 0xFFU)
            // {
            //     data_can[7] = 0U;
            // }
            // can_send_ext(0x01234567U, data_can, 8U);
            // nrf_send_data(data, NRF24_PAYLOAD_SIZE);
            // spi1_dma_transfer(tx, rx, 4U, 0U);

            // uint8_t x = 0xAA;
            // i2c1_write(0x32, &x, 1);

            // uint8_t data = 0U;
            // i2c1_read(0x32U, &data, 1U);

            // if (tca9548_set_channel(3) == 0U || sht40_data_read_int(&sht40_result.temperature, &sht40_result.humidity) == 0U)
            // {
            //     sht40_result.temperature = 0U;
            //     sht40_result.humidity = 0U;
            //     sht40_result.valid = 0U;
            // } else {
            //     sht40_result.valid = 1U;
            // }
            // tca9548_disable_all();

            // if (ai_status != 0U)
            // {
            //     ai_temperature_x100_run(sht40_result.temperature, &ai_class);
            // }

            // if(bme280_read_data(&bme280_result.temperature, &bme280_result.humidity, &bme280_result.pressure)== 0U)
            // {
            //     bme280_result.temperature = 0U;
            //     bme280_result.humidity = 0U;
            //     bme280_result.pressure = 0U;
            //     bme280_result.valid = 0U;
            // } else {
            //     bme280_result.valid = 1U;
            // }

            // if (pin_state == 0) {
            //     pin_state = 1;
            //     mcp23017_set_pin(0, 0, 1);
            // } else {
            //     pin_state = 0;
            //     mcp23017_set_pin(0, 0, 0);
            // }

            pwm_state++;
            if (pwm_state == 1U){
                tim3_ch1_pwm_set_duty(0U);
                // tim3_ch1_pwm_set_frequency(0U);
            } else if (pwm_state == 2U){
                tim3_ch1_pwm_set_duty(25U);
                // tim3_ch1_pwm_set_frequency(1000U);
            } else if (pwm_state == 3U){
                tim3_ch1_pwm_set_duty(50U);
                // tim3_ch1_pwm_set_frequency(2000U);
            } else {
                tim3_ch1_pwm_set_duty(100U);
                // tim3_ch1_pwm_set_frequency(4000U);
                pwm_state = 0U;
            }

            pwm_measurement_t pwm_measurement_local;
            tim4_pwm_get_measurement(&pwm_measurement_local);

            if (yellow_led_state)
            {
                output_off(GPIOF, YELLOW_LED_PIN);
                yellow_led_state = 0U;
            }
            else
            {
                output_on(GPIOF, YELLOW_LED_PIN);
                yellow_led_state = 1U;
            }
        }

        if (compare_1s_flag)
        {
            compare_1s_flag = 0U;
            if (red_led_state){
                output_off(GPIOG, RED_LED_PIN);
                red_led_state = 0U;
            }
            else{
                output_on(GPIOG, RED_LED_PIN);
                red_led_state = 1U;
            }
            
        }

        if (btn1_debounce_done)
        {
            btn1_debounce_done = 0U;
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

void uart2_rx_callback(const uint8_t *data, uint16_t length)
{
    // /* LD2450 */
    // int16_t x;
    // int16_t y;
    // int16_t speed;
    // uint16_t distance_resolution;

    // uint8_t ready = ld2450_process_rx(data, length, &x, &y, &speed, &distance_resolution);
    // if (ready)
    // {
    // }

    /* PROTOCOL */
    static uint8_t rx_frame[PROTOCOL_FRAME_MAX];
    static uint16_t rx_length = 0U;
    static uint16_t expected_length = 0U;
    static uint8_t tx_frame[PROTOCOL_FRAME_MAX];

    for (uint16_t i = 0U; i < length; i++)
    {
        uint8_t byte = data[i];
        

        if (rx_length == 0U)
        {
            if (byte != MAGIC)
            {
                continue;
            }
            rx_frame[rx_length++] = byte;
            continue;
        }

        rx_frame[rx_length++] = byte;
        if (rx_length == 2U)
        {
            if (rx_frame[1] > PROTOCOL_MAX_PAYLOAD)
            {
                rx_length = 0U;
                continue;
            }
            expected_length = 7U + rx_frame[1];
        }

        if ((expected_length > 0U) && (rx_length == expected_length))
        {
            uint8_t valid = 1U;
            if (rx_frame[2] != ADDRESS)
            {
                valid = 0U;
            }
            uint16_t crc = crc16_ccitt(&rx_frame[1],4U + rx_frame[1]);
            uint16_t received_crc = ((uint16_t)rx_frame[5U + rx_frame[1]] << 8) | (uint16_t)rx_frame[6U + rx_frame[1]];

            if (crc != received_crc)
            {
                valid = 0U;
            }

            if (valid)
            {
                protocol_request_t request = {0};

                request.magic = rx_frame[0];
                request.length = rx_frame[1];
                request.address = rx_frame[2];
                request.sequence = rx_frame[3];
                request.command = rx_frame[4];

                for (uint16_t j = 0U; j < request.length; j++)
                {
                    request.data[j] = rx_frame[5U + j];
                }
                uint16_t tx_length = protocol_process_request(&request, tx_frame);
                uart2_dma_write(tx_frame, tx_length);
            }

            rx_length = 0U;
            expected_length = 0U;
        }
    }
}