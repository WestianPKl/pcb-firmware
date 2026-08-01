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
#include "rtc.h"
#include "rng.h"
#include "adc.h"
#include "iwdg.h"
#include "timer.h"
#include "crc.h"
#include "protocol.h"
#include "mcp7940n.h"
#include "tca9548.h"
#include "mcp23017.h"
//#include "mfrc522.h"
// #include <string.h>
// #include "ff.h"
//#include "ssd1306.h"
#include "pcf8563t.h"
//#include "lcd.h"
//#include "e_paper.h"

sht40_data_t sht40_data = {0};
bme280_data_t bme280_data = {0};
rtc_datetime_t rtc_date_time = {0};

static struct netif gnetif;

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
TaskHandle_t task_can_rx_handler = NULL;
TaskHandle_t eth_rx_task_handle = NULL;

void task_init(void *params);
void task_button(void *params);
void task_blink(void *params);
void task_sensor(void *params);
void task_uart2(void *params);
void task_protocol(void *params);
void task_can(void *params);
void task_can_rx(void *params);
void task_eth_rx(void *params);
void task_eth_link(void *params);
void task_tcp_server(void *params);

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

    protocol_queue = xQueueCreate(4, sizeof(protocol_request_t));

    if (protocol_queue == NULL) {
        __disable_irq();
        for(;;) {};
    }

    iwdg_init();

    // lcd_init();
    
    xTaskCreate(task_button, "button", 128, NULL, 2, &task_button_handle);
    xTaskCreate(task_blink, "blink", 128, NULL, 1, NULL);
    xTaskCreate(task_sensor, "sensor", 512, NULL, 3, NULL);
    xTaskCreate(task_uart2, "uart2", 256, NULL, 2, &task_uart2_handle);
    xTaskCreate(task_protocol, "protocol", 256, NULL, 2, NULL);
    xTaskCreate(task_can, "can", 256, NULL, 2, &task_can_handler);
    xTaskCreate(task_can_rx, "can_rx", 256, NULL, 2, &task_can_rx_handler);
    xTaskCreate(task_init, "init", 512, NULL, 4, NULL);

    __enable_irq();

    vTaskStartScheduler();

    for(;;) {};
}

void task_init(void *params)
{
    xTaskCreate(task_eth_rx, "eth_rx", 256, NULL, 3, &eth_rx_task_handle);
    xTaskCreate(task_eth_link, "eth_link", 256, NULL, 3, NULL);
    lwip_start();
    xTaskCreate(task_tcp_server, "tcp", 512, NULL, 3, NULL);
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
    rtc_datetime_t pcf_datetime = {.seconds = 0, .minutes = 0, .hours = 0, .weekday = 0, .day = 0, .month = 0, .year = 0};
    rtc_datetime_t mcp_datetime = {.seconds = 0, .minutes = 0, .hours = 0, .weekday = 0, .day = 0, .month = 0, .year = 0};
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
    uint8_t pin_state = 0;

    if(xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        bme280_init();
    //     uint8_t v;
    //     for (int i = 0; i < 5; i++) {
    //         mfrc522_version(&v);
    //         printf("VERSION[%d] = 0x%02X\n", i, v);
    //     }
    //     uint8_t back;
    //     mfrc522_check(&back);
    //     printf("write 0x26 -> read 0x%02X\n", back);
    //     mfrc522_init();
        xSemaphoreGive(spi_mutex);
    }

    // e_paper_init();
    // e_paper_clear(EPD_1IN64G_WHITE);
    // vTaskDelay(pdMS_TO_TICKS(2000));
    // static FATFS fs;
    // static FIL fil;
    // FRESULT res;
    // UINT bw;
    // const char *text = "Hello, World!";

    // res = f_mount(&fs, "", 1);
    // if (res == FR_OK) {
    //     res = f_open(&fil, "data.txt", FA_WRITE | FA_CREATE_ALWAYS);
    //     if (res == FR_OK) {
    //         f_write(&fil, text, strlen(text), &bw);
    //         f_close(&fil);
    //     }
    //     f_mount(NULL, "", 0);
    // }


    if (xSemaphoreTake(rtc_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
    {
        rtc_init();
        rtc_set_datetime(&local_datetime);
        xSemaphoreGive(rtc_mutex);
    }

    if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
    {
        mcp7940n_init(0);
        mcp7940n_set_datetime(&local_datetime);
    //     ssd1306_init();
    //     ssd1306_clear_display();
    //     ssd1306_display_text("ABC", 0, 0);
    //     ssd1306_display_text("DEF", 0, 1);
        if (tca9548_set_channel(2) == 1) {
            pcf8563t_init();
            pcf8563t_set_datetime(&local_datetime);
        }
        tca9548_disable_all();
        mcp23017_init();
        mcp23017_set_pin_direction(0, 0, 0);
        xSemaphoreGive(i2c_mutex);
    }

    for(;;)
    {
        sht40_data_local.temperature = 0;
        sht40_data_local.humidity = 0;

        bme280_data_local.temperature = 0;
        bme280_data_local.humidity = 0;
        bme280_data_local.pressure = 0;

        // uint32_t rng_value = 0;
        // if (rng_get(&rng_value)) {
        //     printf("RNG value: %lu\n", rng_value);
        // }

        if (xSemaphoreTake(rtc_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        {
            rtc_get_datetime(&rtc_datetime_local);
            xSemaphoreGive(rtc_mutex);
        }

        if (xSemaphoreTake(i2c_mutex, pdMS_TO_TICKS(100)) == pdTRUE)
        {
            if (tca9548_set_channel(3) != 1 ||
                sht40_data_read_int(&sht40_data_local.temperature, &sht40_data_local.humidity) != 0)
            {
                sht40_data_local.temperature = 0;
                sht40_data_local.humidity = 0;
            }
            tca9548_disable_all();
            mcp7940n_get_datetime(&mcp_datetime);
            if (tca9548_set_channel(2) == 1) {
                pcf8563t_get_datetime(&pcf_datetime);
            }
            tca9548_disable_all();

            if (pin_state == 0) {
                pin_state = 1;
                mcp23017_set_pin(0, 0, 1);
            } else {
                pin_state = 0;
                mcp23017_set_pin(0, 0, 0);
            }
            // lcd_set_cursor(0, 0);
            // lcd_send_string("Temp: ");
            // lcd_send_temp_1dp_from_x100(sht40_data_local.temperature);
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

        if (xSemaphoreTake(sensor_reading_mutex, pdMS_TO_TICKS(10)) == pdTRUE) 
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
        
        // printf("RTC: %04d-%02d-%02d %02d:%02d:%02d\n", rtc_date_time.year, rtc_date_time.month, rtc_date_time.day, rtc_date_time.hours, rtc_date_time.minutes, rtc_date_time.seconds);
        // printf("BME280: Temp: %.2f C, Hum: %.2f %%, Pres: %.2f hPa\n", bme280_data.temperature / 100.0f, bme280_data.humidity / 100.0f, bme280_data.pressure / 100.0f);
        // printf("SHT40: Temp: %.2f C, Hum: %.2f %%\n", sht40_data.temperature / 100.0f, sht40_data.humidity / 100.0f);
        // printf("MCP7940N RTC: %04d-%02d-%02d %02d:%02d:%02d\n", mcp_datetime.year, mcp_datetime.month, mcp_datetime.day, mcp_datetime.hours, mcp_datetime.minutes, mcp_datetime.seconds);
        // printf("PCF8563T RTC: %04d-%02d-%02d %02d:%02d:%02d\n", pcf_datetime.year, pcf_datetime.month, pcf_datetime.day, pcf_datetime.hours, pcf_datetime.minutes, pcf_datetime.seconds);


        // if(xSemaphoreTake(spi_mutex, pdMS_TO_TICKS(100)) == pdTRUE) 
        // {
        //     uint8_t atqa[2];
        //     uint8_t atqa_len = 2;

        //     int r = mfrc522_check_atqa(atqa, &atqa_len);
        //     if (r == 0 && atqa_len >= 2) {
        //         uint8_t uid[10];
        //         uint8_t uid_len = 10;
        //         mfrc522_read_id(uid, &uid_len);
        //         printf("UID: ");
        //         for (uint8_t i = 0; i < uid_len; i++) {
        //             printf("%02X ", uid[i]);
        //         }
        //         printf("\n");
        //     }
        //     xSemaphoreGive(spi_mutex);
        // }

        vTaskDelay(pdMS_TO_TICKS(1000));
    }
}

void task_uart2(void *params)
{
    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            uint16_t n = uart2_rx_len;
            if (n > uart2_buffer_size) n = uart2_buffer_size;

            uint8_t valid = (n >= 7U)
                         && (uart2_data[0] == MAGIC)
                         && (uart2_data[1] <= PROTOCOL_MAX_PAYLOAD)
                         && (n == (uint16_t)(uart2_data[1] + 7U))
                         && (uart2_data[2] == ADDRESS);

            if (valid) {
                uint16_t crc = crc16_ccitt(&uart2_data[1], 4U + uart2_data[1]);
                valid = (crc == (uint16_t)((uart2_data[5 + uart2_data[1]] << 8) | uart2_data[6 + uart2_data[1]]));
            }

            protocol_request_t c;
            if (valid) {
                c.magic = uart2_data[0];
                c.length = uart2_data[1];
                c.address = uart2_data[2];
                c.sequence = uart2_data[3];
                c.command = uart2_data[4];
                for (uint16_t i = 0; i < c.length; i++) {
                    c.data[i] = uart2_data[5 + i];
                }
            }

            uart2_dma1_rx_start(uart2_data, uart2_buffer_size);

            if (valid) {
                xQueueSend(protocol_queue, &c, 0);
            }
        }
    }
}

void task_protocol(void *params)
{
    protocol_request_t msg;
    static uint8_t frame[PROTOCOL_FRAME_MAX];
    for (;;) {
        if (xQueueReceive(protocol_queue, &msg, portMAX_DELAY) == pdTRUE) {
            xSemaphoreTake(uart2_mutex, portMAX_DELAY);
            for (uint32_t t = 0U; uart2_tx_busy && (t < 50U); t++) {
                vTaskDelay(1);
            }
            if (uart2_tx_busy) {
                uart2_tx_busy = 0;
            }
            uint16_t len = protocol_process_request(&msg, frame);
            uart2_write_dma(frame, len);
            xSemaphoreGive(uart2_mutex);
        }
    }
}

void task_can(void *params)
{
    TickType_t last_wake = xTaskGetTickCount();
    bme280_data_t bme280_data_local = {.temperature = 0, .humidity = 0, .pressure = 0};
    sht40_data_t sht40_data_local = {.temperature = 0, .humidity = 0};
    rtc_datetime_t rtc_datetime_local = {.seconds = 0, .minutes = 0, .hours = 0, .weekday = 0, .day = 0, .month = 0, .year = 0};

    for (;;) {
    static uint8_t slot = 0;

    switch (slot) {
            case 0:
                send_frame_0x10();
                break;
            case 1:
                if (xSemaphoreTake(sensor_reading_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                    bme280_data_local.temperature = bme280_data.temperature;
                    bme280_data_local.humidity = bme280_data.humidity;
                    bme280_data_local.pressure = bme280_data.pressure;
                    xSemaphoreGive(sensor_reading_mutex);
                }
                    send_frame_0x30(&bme280_data_local);

                break;
            case 2:
                if (xSemaphoreTake(sensor_reading_mutex, pdMS_TO_TICKS(5)) == pdTRUE) {
                    sht40_data_local.temperature = sht40_data.temperature;
                    sht40_data_local.humidity = sht40_data.humidity;
                    xSemaphoreGive(sensor_reading_mutex);
                }
                    send_frame_0x31(&sht40_data_local, &adc_data_buffer[0]);
                break;
            case 3: 
                if (xSemaphoreTake(sensor_reading_mutex, pdMS_TO_TICKS(5)) == pdTRUE) 
                {
                    rtc_datetime_local.seconds = rtc_date_time.seconds;
                    rtc_datetime_local.minutes = rtc_date_time.minutes;
                    rtc_datetime_local.hours = rtc_date_time.hours;
                    rtc_datetime_local.weekday = rtc_date_time.weekday;
                    rtc_datetime_local.day = rtc_date_time.day;
                    rtc_datetime_local.month = rtc_date_time.month;
                    rtc_datetime_local.year = rtc_date_time.year;
                    xSemaphoreGive(sensor_reading_mutex);
                }
                send_frame_0x32(&rtc_datetime_local);
                break;
            default:
                break;
        }

        slot++;
        if (slot >= 4) {
            slot = 0;
        }

        vTaskDelayUntil(&last_wake, pdMS_TO_TICKS(100));
    }
}

void task_can_rx(void *params)
{
    for (;;) {
        if (ulTaskNotifyTake(pdTRUE, portMAX_DELAY)) {
            // printf("CAN RX: ID=0x%04lX \n", can_rx_header.std_id);
            // for (uint8_t i = 0; i < 8; i++) {
            //     printf("CAN RX[%d]: 0x%02X\n", i, can_rx_data[i]);
            // }
            // printf("\n");
        }

        vTaskDelay( pdMS_TO_TICKS(10));
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
            static uint8_t tcp_frame[PROTOCOL_FRAME_MAX];
            do {
                netbuf_data(buf, &data, &len);
                uint8_t *d = (uint8_t *)data;

                uint8_t valid = (len >= 7U)
                             && (d[0] == MAGIC)
                             && (d[1] <= PROTOCOL_MAX_PAYLOAD)
                             && (len == (u16_t)(d[1] + 7U))
                             && (d[2] == ADDRESS);

                if (valid) {
                    uint16_t crc = crc16_ccitt(&d[1], 4U + d[1]);
                    valid = (crc == (uint16_t)((d[5 + d[1]] << 8) | d[6 + d[1]]));
                }

                if (!valid) {
                    continue;
                }

                protocol_request_t c;
                c.magic = d[0];
                c.length = d[1];
                c.address = d[2];
                c.sequence = d[3];
                c.command = d[4];
                for (uint8_t i = 0; i < c.length; i++) {
                    c.data[i] = d[5 + i];
                }

                uint16_t rlen = protocol_process_request(&c, tcp_frame);
                netconn_write(newconn, tcp_frame, rlen, NETCONN_COPY);

            } while (netbuf_next(buf) >= 0);
            netbuf_delete(buf);
        }
        netconn_close(newconn);
        netconn_delete(newconn);
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
