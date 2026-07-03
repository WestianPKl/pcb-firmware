#include "stm32f7xx.h"
#include <stdio.h>
#include "support.h"
#include "outputs.h"
#include "uart.h"
#include "dma.h"
#include "can.h"
#include "main.h"
#include "inputs.h"
#include "lwip/tcpip.h"
#include "lwip/netif.h"
#include "lwip/ip4_addr.h"
#include "lwip/apps//mqtt.h"
#include "netif/etharp.h"
#include "ethernetif.h"
#include "lwip/api.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"
#include "bme280.h"
#include "sht40.h"
#include "rtc.h"
#include "rng.h"
#include "adc.h"
#include "iwdg.h"
#include "timer.h"
#include "crc.h"

sht40_data_t sht40_data = {0};
bme280_data_t bme280_data = {0};
rtc_datetime_t rtc_date_time = {0};

static uint8_t uart2_tx_buf[uart2_buffer_size];

static struct netif gnetif;

uint32_t tx_mailbox;

typedef struct {
    uint8_t address;
    uint8_t cmd;
    uint8_t param;
    uint8_t len;
    uint8_t data[256];
    uint8_t crc;
    uint8_t uart_flag;
} protocol_message_t;

QueueHandle_t protocol_queue;

SemaphoreHandle_t spi_mutex;
SemaphoreHandle_t i2c_mutex;
SemaphoreHandle_t uart2_mutex;
SemaphoreHandle_t rtc_mutex;
SemaphoreHandle_t sensor_reading_mutex;
SemaphoreHandle_t printf_mutex;

TaskHandle_t task_button_handle = NULL;
TaskHandle_t task_uart2_handle = NULL;
TaskHandle_t task_can_handler = NULL;
TaskHandle_t eth_rx_task_handle = NULL;

void task_init(void *params);
void task_button(void *params);
void task_blink(void *params);
void task_sensor(void *params);
void task_uart2(void *params);
void task_protocol(void *params);
void task_can(void *params);
void task_eth_rx(void *params);
void task_eth_link(void *params);
void task_tcp_server(void *params);
void task_mtqq(void *params);

static void clock_init_216mhz(void)
{
    RCC->CR |= RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {}

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR1 |= PWR_CR1_VOS;
    PWR->CR1 |= PWR_CR1_ODEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODRDY)) {}
    PWR->CR1 |= PWR_CR1_ODSWEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODSWRDY)) {}

    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN;
    RCC->PLLCFGR =
        (4U   << RCC_PLLCFGR_PLLM_Pos) |
        (216U << RCC_PLLCFGR_PLLN_Pos) |
        (0U   << RCC_PLLCFGR_PLLP_Pos) |
        (9U   << RCC_PLLCFGR_PLLQ_Pos) |
        RCC_PLLCFGR_PLLSRC_HSE;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    SystemCoreClock = 216000000U;
}

static void lwip_start(void)
{
    ip4_addr_t ipaddr, netmask, gw;

    IP4_ADDR(&ipaddr,  192, 168, 18, 50);
    IP4_ADDR(&netmask, 255, 255, 255, 0);
    IP4_ADDR(&gw,      192, 168, 18, 1);

    tcpip_init(NULL, NULL);

    netif_add(&gnetif, &ipaddr, &netmask, &gw, NULL,
              ethernetif_init,
              tcpip_input);

    netif_set_default(&gnetif);
    netif_set_up(&gnetif);
    netif_set_link_up(&gnetif);
}

int main(void)
{   
    __disable_irq();
    clock_init_216mhz();
    
    NVIC_SetPriorityGrouping(3U);

    dwt_init();
    init_leds();

    dma1_init();
    uart3_rxtx_init();
    uart2_rxtx_init();
    uart3_dma1_rx_start(uart3_data, uart3_buffer_size);
    uart2_dma1_rx_start(uart2_data, uart2_buffer_size);

    dma2_init();
    adc1_ch4_continuous(1);
    adc1_dma2_config(1, adc_data_buffer, ADC_BUFFER_SIZE);
    adc1_start_conversion();

    spi1_init();
    spi1_dma_rx_init();
    spi1_dma_tx_init();

    i2c2_master_config();

    init_timer_tim4_ch2_pwm();

    can_params_init(CAN_MODE_LOOPBACK);
    can_filter_accept_all();
    can_start();

    __enable_irq();

    i2c_mutex = xSemaphoreCreateMutex();
    spi_mutex = xSemaphoreCreateMutex();
    uart2_mutex = xSemaphoreCreateMutex();
    sensor_reading_mutex = xSemaphoreCreateMutex();
    rtc_mutex = xSemaphoreCreateMutex();
    printf_mutex = xSemaphoreCreateMutex();

    if ((i2c_mutex == NULL) || (spi_mutex == NULL) || (uart2_mutex == NULL) || (sensor_reading_mutex == NULL) || (printf_mutex == NULL) || (rtc_mutex == NULL)) {
        __disable_irq();
        for(;;) {};
    }

    protocol_queue = xQueueCreate(4, sizeof(protocol_message_t));

    if (protocol_queue == NULL) {
        __disable_irq();
        for(;;) {};
    }

    iwdg_init();
    
    xTaskCreate(task_button, "button", 128, NULL, 2, &task_button_handle);
    xTaskCreate(task_blink, "blink", 128, NULL, 1, NULL);
    xTaskCreate(task_sensor, "sensor", 512, NULL, 3, NULL);
    xTaskCreate(task_uart2, "uart2", 256, NULL, 2, &task_uart2_handle);
    xTaskCreate(task_protocol, "protocol", 256, NULL, 2, NULL);
    xTaskCreate(task_can, "can", 256, NULL, 2, &task_can_handler);
    xTaskCreate(task_init, "init", 512, NULL, 4, NULL);
    
    vTaskStartScheduler();

    for(;;) {};
}

void task_init(void *params)
{
    xTaskCreate(task_eth_rx, "eth_rx", 256, NULL, 3, &eth_rx_task_handle);
    xTaskCreate(task_eth_link, "eth_link", 256, NULL, 3, NULL);
    lwip_start();
    xTaskCreate(task_tcp_server, "tcp", 512, NULL, 3, NULL);
    xTaskCreate(task_mtqq, "mtqq", 512, NULL, 3, NULL);
    vTaskDelete(NULL);
}

void task_button(void *params)
{
    init_btn();
    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            vTaskDelay(pdMS_TO_TICKS(10));
            GPIOB->ODR ^= (1U << LED3);
            vTaskDelay(pdMS_TO_TICKS(10));
        }
    }
}

void task_blink(void *params)
{
    for (;;) {
        GPIOB->ODR ^= (1U << LED1);
        iwdg_refresh();
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_sensor(void *params)
{
    sht40_data_t sht40_data_local = {.temperature = 0, .humidity = 0};
    bme280_data_t bme280_data_local = {.temperature = 0, .humidity = 0, .pressure = 0};
    rtc_datetime_t rtc_datetime_local = {.seconds = 0, .minutes = 0, .hours = 0, .weekday = 0, .day = 0, .month = 0, .year = 0};

    bme280_init();

    rtc_init();

    rtc_datetime_t local_datetime = 
    {
        .seconds = 0,
        .minutes = 30,
        .hours = 18,
        .weekday = 6,
        .day = 3,
        .month = 7,
        .year = 2026

    };
    rtc_set_datetime(&local_datetime);

    for(;;)
    {
        sht40_data_local.temperature = 0;
        sht40_data_local.humidity = 0;

        bme280_data_local.temperature = 0;
        bme280_data_local.humidity = 0;
        bme280_data_local.pressure = 0;

        uint32_t rng_value = 0;
        if (rng_get(&rng_value)) {
            printf("RNG value: %lu\n", rng_value);
        }

        if (xSemaphoreTake(rtc_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            rtc_get_datetime(&rtc_datetime_local);
            xSemaphoreGive(rtc_mutex);
        }

        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            if (sht40_data_read_int(&sht40_data_local.temperature, &sht40_data_local.humidity) != 0)
            {
                sht40_data_local.temperature = 0;
                sht40_data_local.humidity = 0;
            }
            xSemaphoreGive(i2c_mutex);
        }

        if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            bme280_trigger_forced();
            xSemaphoreGive(spi_mutex);
        }
        vTaskDelay(pdMS_TO_TICKS(10));
        if (xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            if (bme280_read_data(&bme280_data_local.temperature, &bme280_data_local.humidity, &bme280_data_local.pressure) != 0)
            {
                bme280_data_local.temperature = 0;
                bme280_data_local.humidity = 0;
                bme280_data_local.pressure = 0;
            }
            xSemaphoreGive(spi_mutex);
        }

        if (xSemaphoreTake(sensor_reading_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            bme280_data.temperature = bme280_data_local.temperature;
            bme280_data.humidity = bme280_data_local.humidity;
            bme280_data.pressure = bme280_data_local.pressure;

            sht40_data.temperature = sht40_data_local.temperature;
            sht40_data.humidity = sht40_data_local.humidity;

            rtc_date_time.seconds = rtc_datetime_local.seconds;
            rtc_date_time.minutes = rtc_datetime_local.minutes;
            rtc_date_time.hours = rtc_datetime_local.hours;
            rtc_date_time.weekday = rtc_datetime_local.weekday;
            rtc_date_time.day = rtc_datetime_local.day;
            rtc_date_time.month = rtc_datetime_local.month;
            rtc_date_time.year = rtc_datetime_local.year;
            xSemaphoreGive(sensor_reading_mutex);
        }
        
        printf("RTC: %04d-%02d-%02d %02d:%02d:%02d\n", rtc_date_time.year, rtc_date_time.month, rtc_date_time.day, rtc_date_time.hours, rtc_date_time.minutes, rtc_date_time.seconds);
        printf("BME280: Temp: %.2f C, Hum: %.2f %%, Pres: %.2f hPa\n", bme280_data.temperature / 100.0f, bme280_data.humidity / 100.0f, bme280_data.pressure / 100.0f);
        printf("SHT40: Temp: %.2f C, Hum: %.2f %%\n", sht40_data.temperature / 100.0f, sht40_data.humidity / 100.0f);
        
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_uart2(void *params)
{
    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            uint16_t n = uart2_rx_len;
            if (n > uart2_buffer_size) n = uart2_buffer_size;

            for (uint16_t i = 0; i < n; i++)
                uart2_tx_buf[i] = uart2_data[i];

            if (n < 5U) {
                uart2_dma1_rx_start(uart2_data, uart2_buffer_size);
                continue;
            }

            protocol_message_t c;
            c.address = uart2_tx_buf[0];
            c.cmd     = uart2_tx_buf[1];
            c.param   = uart2_tx_buf[2];
            c.len     = uart2_tx_buf[3];
            c.uart_flag = 1;

            if ((uint32_t)4U + c.len + 1U > (uint32_t)n) {
                uart2_dma1_rx_start(uart2_data, uart2_buffer_size);
                continue;
            }

            for (uint8_t i = 0; i < c.len; i++) {
                c.data[i] = uart2_tx_buf[4 + i];
            }
            c.crc = uart2_tx_buf[4 + c.len];

            uart2_dma1_rx_start(uart2_data, uart2_buffer_size);

            xQueueSend(protocol_queue, &c, 0);
        }
    }
}

void task_protocol(void *params)
{
    // [HEADER] [CMD] [PARAM] [LEN] [DATA] [CRC]
    // HEADER: 0xAA
    // CMD: 0x01 - SET PWM
    // PARAM: 0x01 - PWM Channel 1
    // LEN: 0x01 - Length of DATA
    // DATA: 0x64 - PWM value (0-100)
    // CRC: 0x?? - CRC8 of CMD, PARAM, LEN, DATA

    protocol_message_t msg;
    for (;;) {
        if (xQueueReceive(protocol_queue, &msg, portMAX_DELAY) == pdTRUE) {
            switch(msg.cmd) {
                case 0x00: 
                {
                    uint8_t response[3] = {0xAA, 0xAA, 0xAA};

                    if(msg.uart_flag) {
                        if (xSemaphoreTake(uart2_mutex, portMAX_DELAY) == pdTRUE) {
                            uart2_write_dma(response, 3);
                            xSemaphoreGive(uart2_mutex);
                        }
                    }
                    break;
                }
                case 0x01: 
                {
                    if (msg.param == 0x01 && msg.len == 1) {
                        uint8_t pwm_value = msg.data[0];
                        if (pwm_value > 100) pwm_value = 100;
                        set_duty_timer_tim4_ch2_pwm(pwm_value);
                        
                        uint8_t response[5] = {0xAA, 0x01, 0x01, pwm_value, 0x00};
                        response[4] = crc8_atm(&response[1], 4);
                        if(msg.uart_flag) {
                            if (xSemaphoreTake(uart2_mutex, portMAX_DELAY) == pdTRUE) {
                                uart2_write_dma(response, 5);
                                xSemaphoreGive(uart2_mutex);
                            }
                        }
                    }
                    break;
                }
            }
        }
    }
}

void task_can(void *params)
{
    for (;;) {
        // Cyclic Check
        can_tx_header_typedef tx_header = {0};
        uint8_t tx_data[4] = {0x10, 0x20, 0x30, 0x40};
        tx_header.std_id = 0x10;
        tx_header.ide = CAN_ID_STD;
        tx_header.dlc = 4;

        can_add_tx_message(&tx_header, tx_data, &tx_mailbox);

        // Cyclic Temperature and Humidity Read
        // if (xSemaphoreTake(sensor_reading_mutex, pdMS_TO_TICKS(100)) == pdTRUE) {
        //     can_tx_header_typedef sensor_header_bme = {0};
        //     uint8_t sensor_data_bme[6] = {0};
        //     sensor_header_bme.std_id = 0x20;
        //     sensor_header_bme.ide = CAN_ID_STD;
        //     sensor_header_bme.dlc = 6;

        //     sensor_data_bme[0] = (bme280_data.temperature >> 8) & 0xFF;
        //     sensor_data_bme[1] = bme280_data.temperature & 0xFF;
        //     sensor_data_bme[2] = (bme280_data.humidity >> 8) & 0xFF;
        //     sensor_data_bme[3] = bme280_data.humidity & 0xFF;
        //     sensor_data_bme[4] = (bme280_data.pressure >> 8) & 0xFF;
        //     sensor_data_bme[5] = bme280_data.pressure & 0xFF;

        //     can_add_tx_message(&sensor_header_bme, sensor_data_bme, &tx_mailbox);

        //     can_tx_header_typedef sensor_header_sht = {0};
        //     uint8_t sensor_data_sht[4] = {0};
        //     sensor_header_sht.std_id = 0x30;
        //     sensor_header_sht.ide = CAN_ID_STD;
        //     sensor_header_sht.dlc = 4;

        //     sensor_data_sht[0] = (sht40_data.temperature >> 8) & 0xFF;
        //     sensor_data_sht[1] = sht40_data.temperature & 0xFF;
        //     sensor_data_sht[2] = (sht40_data.humidity >> 8) & 0xFF;
        //     sensor_data_sht[3] = sht40_data.humidity & 0xFF;

        //     can_add_tx_message(&sensor_header_sht, sensor_data_sht, &tx_mailbox);
        //     xSemaphoreGive(sensor_reading_mutex);
        // }

        if (ulTaskNotifyTake(pdTRUE, pdMS_TO_TICKS(100)) != 0) {
            printf("Received 0: %d\n", can_rx_data[0]);
        }
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_eth_rx(void *params)
{
    for (;;) {
        ulTaskNotifyTake(pdTRUE, portMAX_DELAY);
        ethernetif_input(&gnetif);
    }
}

void task_eth_link(void *params)
{
    uint8_t last = 0xFF;

    for (;;) {
        uint8_t now = eth_link_up();

        if (now != last) {
            if (now) {
                netif_set_link_up(&gnetif);
            } else {
                netif_set_link_down(&gnetif);
            }
            last = now;
        }
        vTaskDelay(pdMS_TO_TICKS(500));
    }
}

void task_tcp_server(void *params)
{
    struct netconn *conn, *newconn;
    err_t err;

    conn = netconn_new(NETCONN_TCP);
    if (conn == NULL) { vTaskDelete(NULL); return; }

    netconn_bind(conn, IP_ADDR_ANY, 1234);

    netconn_listen(conn);

    for (;;) {
        err = netconn_accept(conn, &newconn);
        if (err != ERR_OK) continue;

        struct netbuf *buf;
        while (netconn_recv(newconn, &buf) == ERR_OK) {
            void *data;
            u16_t len;
            do {
                netbuf_data(buf, &data, &len);
                netconn_write(newconn, data, len, NETCONN_COPY);
                protocol_message_t c;

                c.address = ((uint8_t *)data)[0];
                c.cmd     = ((uint8_t *)data)[1];
                c.param   = ((uint8_t *)data)[2];
                c.len     = ((uint8_t *)data)[3];
                c.uart_flag = 0;

                if ((uint32_t)4U + c.len + 1U > (uint32_t)len) {
                    break;
                }
                for (uint8_t i = 0; i < c.len; i++) {
                    c.data[i] = ((uint8_t *)data)[4 + i];
                }
                c.crc = ((uint8_t *)data)[4 + c.len];
                xQueueSend(protocol_queue, &c, 0);
                

            } while (netbuf_next(buf) >= 0);
            netbuf_delete(buf);
        }
        netconn_close(newconn);
        netconn_delete(newconn);
    }
}

void task_mtqq(void *params)
{
    for (;;) {
        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void vApplicationStackOverflowHook(TaskHandle_t xTask, char *pcTaskName)
{
    (void)xTask;
    (void)pcTaskName;
    taskDISABLE_INTERRUPTS();
    for(;;) {};
}

void vApplicationMallocFailedHook(void)
{
    taskDISABLE_INTERRUPTS();
    for(;;) {};
}
