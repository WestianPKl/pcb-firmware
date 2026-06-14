#include <stdint.h>
#include <string.h>
#include "uart_protocol.h"
#include "uart.h"
#include "dma.h"
#include "adc.h"
#include "lcd.h"
#include "sht40.h"
#include "bme280.h"
#include "rtc.h"
#include "ina.h"
#include "timer.h"
#include "outputs.h"
#include "inputs.h"
#include "spi.h"
#include "i2c.h"
#include "systick.h"
#include "support.h"
#include "app_flags.h"
#include "mcp7940n.h"
#include "fm24cl16b.h"
#include "mx25l25673gm2i.h"
#include "version.h"
#include "flash_log.h"
#include "ld2450.h"

#define STATUS_OK       0x40
#define ERROR_RESPONSE  0x7F

volatile uint8_t uart2_tx_busy;
volatile uint8_t uart1_tx_busy;

uint8_t uart2_rx_buf[UART2_RX_BUFFER_SIZE];
uint8_t uart2_tx_frame[FRAME_LEN_APP];
volatile uint16_t uart2_rx_old_pos;

uint8_t uart2_frame_acc[UART2_RX_FRAME_LEN];
volatile uint16_t uart2_frame_idx;

uint8_t uart1_rx_buf[UART1_RX_BUFFER_SIZE];
uint8_t uart1_tx_frame[FRAME_LEN_APP];
volatile uint16_t uart1_rx_old_pos;

uint8_t uart1_frame_acc[UART1_RX_FRAME_LEN];
volatile uint16_t uart1_frame_idx;

static int uart2_dma_send(const uint8_t *data, uint16_t len);
static int uart1_dma_send(const uint8_t *data, uint16_t len);
static void handle_request(const uint8_t *req, uint8_t use_uart1);


static int uart2_dma_send(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return -1;
    if (len > sizeof(uart2_tx_frame)) len = sizeof(uart2_tx_frame);

    if (uart2_tx_busy) {
        return -1;
    }

    uart2_tx_busy = 1;
    memcpy(uart2_tx_frame, data, len);
    dma1_uart2_tx_start(uart2_tx_frame, len);
    return 1;
}

static int uart1_dma_send(const uint8_t *data, uint16_t len)
{
    if (!data || len == 0) return -1;
    if (len > sizeof(uart1_tx_frame)) len = sizeof(uart1_tx_frame);

    if (uart1_tx_busy) {
        return -1;
    }

    uart1_tx_busy = 1;
    memcpy(uart1_tx_frame, data, len);
    dma2_uart1_tx_start(uart1_tx_frame, len);
    return 1;
}

static void handle_response(uint8_t status, uint8_t cmd, uint8_t param,
                            const uint8_t *payload, uint32_t payload_len, uint8_t use_uart1)
{
    uint8_t resp[FRAME_LEN_APP];
    memset(resp, 0, sizeof(resp));

    if (payload_len > FRAME_PAYLOAD) {
        return;
    }

    resp[0] = DEV_ADDR;
    resp[1] = status;
    resp[2] = cmd;
    resp[3] = param;

    if (payload && payload_len) {
        memcpy(&resp[4], payload, payload_len);
    }

    resp[FRAME_LEN_APP - 1] = crc8_atm(resp, FRAME_LEN_APP - 1);

    if (use_uart1) uart1_dma_send(resp, FRAME_LEN_APP);
    else           uart2_dma_send(resp, FRAME_LEN_APP);
}

static void handle_request(const uint8_t *req, uint8_t use_uart1)
{
    uint8_t addr  = req[0];
    uint8_t cmd   = req[2];
    uint8_t param_addr = req[3];
    uint16_t cmd_combined = ((uint16_t)cmd << 8) | param_addr;

    if (addr != DEV_ADDR) return;

    switch (cmd_combined) {
        case 0x0000: /* No operation */
        {
            uint8_t data[3] = {0xAA, 0xAA, 0xAA};
            handle_response(STATUS_OK, cmd, param_addr, data, sizeof(data), use_uart1);
            break;
        }

        case 0x0100: /*READ: Get device serial number */
        {
            const device_info_t *info = device_info_get();
            uint32_t serial = 0;
            if (info->magic == INFO_MAGIC) serial = info->serial;

            uint8_t serial_bytes[4] = {
                (uint8_t)((serial >> 24) & 0xFF),
                (uint8_t)((serial >> 16) & 0xFF),
                (uint8_t)((serial >> 8) & 0xFF),
                (uint8_t)(serial & 0xFF)
            };
            handle_response(STATUS_OK, cmd, param_addr, serial_bytes, 4, use_uart1);
            break;
        }

        case 0x0101: /*READ: Get firmware and hardware version */
        {
            const device_info_t *info = device_info_get();
            uint8_t hwmaj = 0, hwmin = 0;
            if (info->magic == INFO_MAGIC) { hwmaj = info->hw_major; hwmin = info->hw_minor; }

            uint8_t payload[5] = {
                FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH, hwmaj, hwmin
            };
            handle_response(STATUS_OK, cmd, param_addr, payload, 5, use_uart1);
            break;
        }

        case 0x0102: /*READ: Get firmware build date */
        {
            uint8_t payload[10] = {0};
            memcpy(payload, FW_BUILD_DATE, 10);
            handle_response(STATUS_OK, cmd, param_addr, payload, 10, use_uart1);
            break;
        }

        case 0x0103: /*READ: Get production date */
        {
            const device_info_t *info = device_info_get();
            uint8_t payload[10] = {0};
            if (info->magic == INFO_MAGIC) memcpy(payload, info->prod_date, 10);
            handle_response(STATUS_OK, cmd, param_addr, payload, 10, use_uart1);
            break;
        }

        case 0x0200: /*READ: Get ADC measurements (up to 4 channels, 16-bit each) */
        {
            if (!adc_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (ADC_BUFFER_SIZE < 2) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }
            uint16_t v0 = adc_data_buffer[0];
            uint16_t v1 = adc_data_buffer[1];
            uint8_t data[4] = {(uint8_t)(v0 >> 8), (uint8_t)v0, (uint8_t)(v1 >> 8), (uint8_t)v1};
            handle_response(STATUS_OK, cmd, param_addr, data, 4, use_uart1);
            break;
        }

        case 0x0201: /*READ: Get button states and ESP32 input */
        {
            uint8_t btn_pressed = GPIOB->IDR & (1U << 0U) ? 0U : 1U;
            uint8_t data[1] = { btn_pressed ? 1U : 0U };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0202: /*READ: Get button states and ESP32 input */
        {
            uint8_t btn_pressed = GPIOB->IDR & (1U << 1U) ? 0U : 1U;
            uint8_t data[1] = { btn_pressed ? 1U : 0U };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0203: /*READ: Get button states and ESP32 input */
        {
            uint8_t esp32_input = (GPIOC->IDR & (1U << 5U)) ? 1U : 0U;
            uint8_t data[1] = { esp32_input ? 1U : 0U };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0300: /*READ: Get SHT40 temperature and humidity */
        {

            if (!sht40_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (sht40_error_flag) {
                uint8_t err = 2;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            int16_t temp_c_x100 = measurement_sht40.temperature;
            uint16_t rh_x100 = measurement_sht40.humidity;

            uint8_t data[4];
            data[0] = (uint8_t)((temp_c_x100 >> 8) & 0xFF);
            data[1] = (uint8_t)(temp_c_x100 & 0xFF);
            data[2] = (uint8_t)((rh_x100 >> 8) & 0xFF);
            data[3] = (uint8_t)(rh_x100 & 0xFF);

            handle_response(STATUS_OK, cmd, param_addr, data, 4, use_uart1);
            break;
        }

        case 0x0301: /*READ: Get BME280 temperature, humidity and pressure */
        {
            if (!bme280_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (bme280_error_flag) {
                uint8_t err = 2;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            int32_t temp_x100   = measurement_bme280.temperature;
            uint32_t hum_x100   = measurement_bme280.humidity;
            uint32_t press_pa   = measurement_bme280.pressure;

            if (temp_x100 < -4000 || temp_x100 > 8500 || hum_x100 > 10000U || press_pa == 0) {
                uint8_t err = 3;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint8_t data[12];
            data[0]  = (uint8_t)((temp_x100 >> 24) & 0xFF);
            data[1]  = (uint8_t)((temp_x100 >> 16) & 0xFF);
            data[2]  = (uint8_t)((temp_x100 >> 8) & 0xFF);
            data[3]  = (uint8_t)(temp_x100 & 0xFF);

            data[4]  = (uint8_t)((hum_x100 >> 24) & 0xFF);
            data[5]  = (uint8_t)((hum_x100 >> 16) & 0xFF);
            data[6]  = (uint8_t)((hum_x100 >> 8) & 0xFF);
            data[7]  = (uint8_t)(hum_x100 & 0xFF);

            data[8]  = (uint8_t)((press_pa >> 24) & 0xFF);
            data[9]  = (uint8_t)((press_pa >> 16) & 0xFF);
            data[10] = (uint8_t)((press_pa >> 8) & 0xFF);
            data[11] = (uint8_t)(press_pa & 0xFF);

            handle_response(STATUS_OK, cmd, param_addr, data, 12, use_uart1);
            break;
        }

        case 0x0302: /*SERVICE: Clear LCD display */
        {
            if (!display_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            lcd_clear();
            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0303: /*SERVICE: Set LCD backlight on/off */
        {
            if (!display_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }
            uint8_t val = req[4] ? 1U : 0U;
            backlight_timer = 0;
            backlight_toggle_flag = 1;
            backlight_on = val ? 1U : 0U;
            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0304: /* READ RADAR DATA */
        {
            uint8_t data[27];
            uint8_t off = 0;
            for (uint8_t i = 0; i < LD2450_MAX_TARGETS; i++) {
                const LD2450_Target *t = &ld2450_targets[i];
                data[off++] = t->valid;
                data[off++] = (uint8_t)((uint16_t)t->x >> 8);
                data[off++] = (uint8_t)((uint16_t)t->x & 0xFFU);
                data[off++] = (uint8_t)((uint16_t)t->y >> 8);
                data[off++] = (uint8_t)((uint16_t)t->y & 0xFFU);
                data[off++] = (uint8_t)((uint16_t)t->speed >> 8);
                data[off++] = (uint8_t)((uint16_t)t->speed & 0xFFU);
                data[off++] = (uint8_t)(t->distance >> 8);
                data[off++] = (uint8_t)(t->distance & 0xFFU);
            }
            handle_response(STATUS_OK, cmd, param_addr, data, sizeof(data), use_uart1);
            break;
        }

        case 0x0400: /*READ: Get LEDs and outputs state (6 bytes: LED1, LED2, RGB R, G, B, Brightness) */
        {
            uint8_t data[6] = { led1_state, led2_state, rgb_r, rgb_g, rgb_b, rgb_brightness };
            handle_response(STATUS_OK, cmd, param_addr, data, 6, use_uart1);
            break;
        }

        case 0x0401: /*OPERATIONAL: Set LED1 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('B', 14U);
            else     pin_set_low('B', 14U);
            
            led1_state = val;
            uint8_t data[1] = { led1_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0402: /*OPERATIONAL: Set LED2 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('B', 15U);
            else     pin_set_low('B', 15U);

            led2_state = val;
            uint8_t data[1] = { led2_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0403: /*OPERATIONAL: Set PB12 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('B', 12U);
            else     pin_set_low('B', 12U);

            pb12_state = val;
            uint8_t data[1] = { pb12_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0404: /*OPERATIONAL: Set PC0 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('C', 0U);
            else     pin_set_low('C', 0U);

            pc0_state = val;
            uint8_t data[1] = { pc0_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0405: /*OPERATIONAL: Set PC1 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('C', 1U);
            else     pin_set_low('C', 1U);

            pc1_state = val;
            uint8_t data[1] = { pc1_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0406: /*OPERATIONAL: Set PC2 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('C', 2U);
            else     pin_set_low('C', 2U);

            pc2_state = val;
            uint8_t data[1] = { pc2_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0407: /*OPERATIONAL: Set PC3 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;

            if (val) pin_set_high('C', 3U);
            else     pin_set_low('C', 3U);

            pc3_state = val;
            uint8_t data[1] = { pc3_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0408: /*OPERATIONAL: Set PC4 state (1 byte: 0 or 1) */
        {
            uint8_t val = req[4] ? 1U : 0U;
            if (val) pin_set_high('C', 4U);
            else     pin_set_low('C', 4U);
            esp32_state = val;
            uint8_t data[1] = { esp32_state };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0501: /*OPERATIONAL: Set PWM duty cycle for timer1 channel 1 (1 byte: 0-100) */
        {
            uint8_t duty = req[4];

            if (duty > 100U) duty = 100U;
            
            timer1_pwm_ch1_set_duty(duty);
            uint8_t data[1] = { duty };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0502: /*OPERATIONAL: Set PWM duty cycle for timer2 channel 3 (1 byte: 0-100) */
        {
            uint8_t duty = req[4];

            if (duty > 100U) duty = 100U;

            timer2_pwm_ch3_set_duty(duty);
            uint8_t data[1] = { duty };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0503: /*OPERATIONAL: Set PWM duty cycle for timer4 channel 3 (1 byte: 0-100) */
        {
            uint8_t duty = req[4];

            if (duty > 100U) duty = 100U;

            timer4_pwm_ch3_set_duty(duty);
            uint8_t data[1] = { duty };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0504: /*OPERATIONAL: Set PWM duty cycle for timer4 channel 4 (1 byte: 0-100) */
        {
            uint8_t duty = req[4];

            if (duty > 100U) duty = 100U;

            timer4_pwm_ch4_set_duty(duty);
            uint8_t data[1] = { duty };
            handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            break;
        }

        case 0x0505: /*OPERATIONAL: Set RGB color for timer3 PWM (3 bytes: R, G, B) */
        {
            rgb_r = req[4];
            rgb_g = req[5];
            rgb_b = req[6];
            rgb_brightness = req[7];

            if (rgb_brightness > 100U) {
                rgb_brightness = 100U;
            }
            
            timer3_pwm_set_color(rgb_r, rgb_g, rgb_b, rgb_brightness);
            uint8_t data[4] = { rgb_r, rgb_g, rgb_b, rgb_brightness };
            handle_response(STATUS_OK, cmd, param_addr, data, 4, use_uart1);
            break;
        }

        case 0x0506: /*OPERATIONAL: Set buzzer frequency and volume for timer8 PWM (3 bytes: freq_H, freq_L, vol) */
        {
            if (req[4] == 0 && req[5] == 0) {
                timer8_pwm_set_buzzer_freq(0, 0);
                handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            uint16_t freq = ((uint16_t)req[4] << 8) | req[5];
            uint8_t  vol  = req[6];

            if (vol > 100U) vol = 100U;

            timer8_pwm_set_buzzer_freq((uint32_t)freq, (uint32_t)vol);
            uint8_t data[3] = { req[4], req[5], vol };
            handle_response(STATUS_OK, cmd, param_addr, data, 3, use_uart1);
            break;
        }

        case 0x0600: /*READ: Get date and time from RTC (7 bytes: year, month, day, weekday, hour, minute, second) */
        {
            uint8_t data[7];

            data[0] = datetime.year;
            data[1] = datetime.month;
            data[2] = datetime.day;
            data[3] = datetime.weekday;
            data[4] = datetime.hours;
            data[5] = datetime.minutes;
            data[6] = datetime.seconds;

            handle_response(STATUS_OK, cmd, param_addr, data, 7, use_uart1);
            break;
        }

        case 0x0601: /*OPERATIONAL: Set date and time to RTC (7 bytes: year, month, day, weekday, hour, minute, second) */
        {
            uint8_t year    = req[4];
            uint8_t month   = req[5];
            uint8_t day     = req[6];
            uint8_t weekday = req[7];
            uint8_t hours   = req[8];
            uint8_t minutes = req[9];
            uint8_t seconds = req[10];

            if (year > 99U ||
                month < 1U || month > 12U ||
                day   < 1U || day   > 31U ||
                weekday < 1U || weekday > 7U ||
                hours > 23U || minutes > 59U || seconds > 59U) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (ext_rtc_present) {
                mcp7940n_datetime_t dt_local;
                dt_local.year  = year;
                dt_local.month = month;
                dt_local.mday  = day;
                dt_local.wday  = weekday;
                dt_local.hour  = hours;
                dt_local.min   = minutes;
                dt_local.sec   = seconds;

                if (mcp7940n_set_datetime(&dt_local) != 1) {
                    handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                    break;
                }
            } else {
                int rc = rtc_set_datetime(year, month, day, weekday, hours, minutes, seconds);

                if (rc != 0) {
                    uint8_t err = (uint8_t)(-rc);
                    handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                    break;
                }
            }

            uint8_t data[7] = { year, month, day, weekday, hours, minutes, seconds };
            handle_response(STATUS_OK, cmd, param_addr, data, 7, use_uart1);
            break;
        }

        case 0x0602: /*OPERATIONAL: Set RTC wakeup timer in seconds (2 bytes: seconds) */
        {
            if (ext_rtc_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint16_t sec = ((uint16_t)req[4] << 8) | req[5];
            if (rtc_wakeup_start_seconds(sec) != 0) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }
            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0603: /*OPERATIONAL: Set RTC alarm A in hours, minutes and seconds (4 bytes: hours, minutes, seconds, daily_flag) */
        {
            if (ext_rtc_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint8_t hh = req[4];
            uint8_t mm = req[5];
            uint8_t ss = req[6];
            uint8_t daily = req[7];

            if (rtc_alarmA_set_hms(hh, mm, ss, daily ? 1U : 0U) != 0) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }
            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0604: /*OPERATIONAL: Disable RTC alarm A */
        {
            if (ext_rtc_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            rtc_alarmA_disable();
            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0605: /*READ: Get RTC timestamp (7 bytes: month, day, weekday, hour, minute, second) */
        {
            if (ext_rtc_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint8_t mo, dd, wd, hh, mi, ss;
            int r = rtc_timestamp_read(&mo, &dd, &wd, &hh, &mi, &ss);
            if (r != 0) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }
            uint8_t data[7] = { 0xFF, mo, dd, wd, hh, mi, ss };
            handle_response(STATUS_OK, cmd, param_addr, data, 7, use_uart1);
            break;
        }

        case 0x0700: /*READ: Get INA226 measurements (bus voltage, shunt voltage, current and power) */
        {
            if (!ina226_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint16_t id = 0, cal = 0;
            int r = ina226_id(&id, &cal);
            if (r < 0) {
                uint8_t err = (uint8_t)(-r);
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }
            uint32_t bus_uV = ina226_bus_uV();
            int32_t shunt_uV = ina226_shunt_uV();
            int32_t current_uA = ina226_current_uA();
            uint32_t power_uW = ina226_power_uW();

            uint8_t data[18];
            data[0] = (bus_uV >> 24) & 0xFF;
            data[1] = (bus_uV >> 16) & 0xFF;
            data[2] = (bus_uV >> 8) & 0xFF;
            data[3] = bus_uV & 0xFF;
            data[4] = (shunt_uV >> 24) & 0xFF;
            data[5] = (shunt_uV >> 16) & 0xFF;
            data[6] = (shunt_uV >> 8) & 0xFF;
            data[7] = shunt_uV & 0xFF;
            data[8] = (current_uA >> 24) & 0xFF;
            data[9] = (current_uA >> 16) & 0xFF;
            data[10] = (current_uA >> 8) & 0xFF;
            data[11] = current_uA & 0xFF;
            data[12] = (power_uW >> 24) & 0xFF;
            data[13] = (power_uW >> 16) & 0xFF;
            data[14] = (power_uW >> 8) & 0xFF;
            data[15] = power_uW & 0xFF;
            data[16] = (id >> 8) & 0xFF;
            data[17] = id & 0xFF;

            handle_response(STATUS_OK, cmd, param_addr, data, 18, use_uart1);
            break;
        }

        case 0x0800: /*SERVICE: Write data to FRAM (up to 16 bytes) */
        {
            uint16_t addr = ((uint16_t)req[4] << 8) | req[5];
            uint8_t data_len = req[6];

            if (data_len == 0 || data_len > 16) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if ((uint32_t)addr + data_len > FM24CL16B_SIZE_BYTES) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (fm24cl16b_write(addr, &req[7], data_len) != 1) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0801: /*SERVICE: Read data from FRAM (up to 16 bytes) */
        {
            uint16_t addr = ((uint16_t)req[4] << 8) | req[5];
            uint8_t data_len = req[6];
            uint8_t data[16] = {0};

            if (data_len == 0 || data_len > 16) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if ((uint32_t)addr + data_len > FM24CL16B_SIZE_BYTES) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (fm24cl16b_read(addr, data, data_len) != 1) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            handle_response(STATUS_OK, cmd, param_addr, data, data_len, use_uart1);
            break;
        }

        case 0x0802: /*READ: Get FRAM configuration flags (1 byte) */
        {
            uint8_t cfg_flags = 0;
            if (fm24cl16b_read_byte(FRAM_ADDR_FLAGS, &cfg_flags) == 1) {
                uint8_t data[1] = { cfg_flags };
                handle_response(STATUS_OK, cmd, param_addr, data, 1, use_uart1);
            } else {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
            }
            break;
        }

        case 0x0803: /*SERVICE: Set FRAM configuration flags (1 byte) */
        {
            uint8_t flags = req[4];
            if (fm24cl16b_write_byte(FRAM_ADDR_FLAGS, flags) == 1) {
                handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
                fram_flags_toggle_flag = 1;
            } else {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
            }
            break;
        }

        case 0x0810: /* SERVICE: Raw write data to SPI flash (up to 15 bytes, no erase) */
        {
            if (!flash_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint32_t addr = ((uint32_t)req[4] << 24) |
                            ((uint32_t)req[5] << 16) |
                            ((uint32_t)req[6] << 8)  |
                            ((uint32_t)req[7]);

            uint8_t data_len = req[8];

            if (data_len == 0u || data_len > 15u) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (addr >= MX25_SIZE_BYTES) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (data_len > (MX25_SIZE_BYTES - addr)) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (mx25_write(addr, &req[9], data_len) != 1) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0811: /* SERVICE: Read data from SPI flash (up to 16 bytes) */
        {
            if (!flash_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint32_t addr = ((uint32_t)req[4] << 24) |
                            ((uint32_t)req[5] << 16) |
                            ((uint32_t)req[6] << 8)  |
                            ((uint32_t)req[7]);

            uint8_t data_len = req[8];
            uint8_t data[16] = {0};

            if (data_len == 0u || data_len > 16u) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (addr >= MX25_SIZE_BYTES) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (data_len > (MX25_SIZE_BYTES - addr)) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            if (mx25_read(addr, data, data_len) != 1) {
                handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
                break;
            }

            handle_response(STATUS_OK, cmd, param_addr, data, data_len, use_uart1);
            break;
        }

        case 0x0812: /* SERVICE: Append log record from external host */
        {
            if (!flash_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            uint32_t timestamp =
                ((uint32_t)req[4] << 24) |
                ((uint32_t)req[5] << 16) |
                ((uint32_t)req[6] << 8)  |
                ((uint32_t)req[7]);

            int32_t temp =
                ((int32_t)req[8]  << 24) |
                ((int32_t)req[9]  << 16) |
                ((int32_t)req[10] << 8)  |
                ((int32_t)req[11]);

            uint32_t hum =
                ((uint32_t)req[12] << 24) |
                ((uint32_t)req[13] << 16) |
                ((uint32_t)req[14] << 8)  |
                ((uint32_t)req[15]);

            uint32_t press =
                ((uint32_t)req[16] << 24) |
                ((uint32_t)req[17] << 16) |
                ((uint32_t)req[18] << 8)  |
                ((uint32_t)req[19]);

            if (press == 0u) {
                measurement_sht40_t sht;

                if (temp < -32768L || temp > 32767L || hum > 65535u) {
                    uint8_t err = 2;
                    handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                    break;
                }

                sht.temperature = (int16_t)temp;
                sht.humidity    = (uint16_t)hum;

                if (flash_log_append(timestamp, 0, &sht) != 1) {
                    uint8_t err = 3;
                    handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                    break;
                }
            } else {
                measurement_bme280_t bme;

                bme.temperature = temp;
                bme.humidity    = hum;
                bme.pressure    = press;

                if (flash_log_append(timestamp, &bme, 0) != 1) {
                    uint8_t err = 3;
                    handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                    break;
                }
            }

            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x0813: /* READ: Get log record by index */
        {
            flash_log_record_t rec;
            uint8_t mode = req[4];
            uint8_t data[24];

            uint32_t index =
                ((uint32_t)req[5] << 24) |
                ((uint32_t)req[6] << 16) |
                ((uint32_t)req[7] << 8)  |
                ((uint32_t)req[8]);

            int rc;

            if (!flash_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (sizeof(flash_log_record_t) > FRAME_PAYLOAD) {
                uint8_t err = 4;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (mode == 0u) {
                rc = flash_log_read_oldest(index, &rec);
            } else if (mode == 1u) {
                rc = flash_log_read_latest(index, &rec);
            } else {
                uint8_t err = 2;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (rc != 1) {
                uint8_t err = 3;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            data[0]  = (uint8_t)(rec.sequence >> 24);
            data[1]  = (uint8_t)(rec.sequence >> 16);
            data[2]  = (uint8_t)(rec.sequence >> 8);
            data[3]  = (uint8_t)(rec.sequence);

            data[4]  = (uint8_t)(rec.timestamp >> 24);
            data[5]  = (uint8_t)(rec.timestamp >> 16);
            data[6]  = (uint8_t)(rec.timestamp >> 8);
            data[7]  = (uint8_t)(rec.timestamp);

            data[8]  = (uint8_t)(((uint32_t)rec.temp_x100) >> 24);
            data[9]  = (uint8_t)(((uint32_t)rec.temp_x100) >> 16);
            data[10] = (uint8_t)(((uint32_t)rec.temp_x100) >> 8);
            data[11] = (uint8_t)(((uint32_t)rec.temp_x100));

            data[12] = (uint8_t)(rec.hum_x100 >> 24);
            data[13] = (uint8_t)(rec.hum_x100 >> 16);
            data[14] = (uint8_t)(rec.hum_x100 >> 8);
            data[15] = (uint8_t)(rec.hum_x100);

            data[16] = (uint8_t)(rec.press_pa >> 24);
            data[17] = (uint8_t)(rec.press_pa >> 16);
            data[18] = (uint8_t)(rec.press_pa >> 8);
            data[19] = (uint8_t)(rec.press_pa);

            data[20] = (uint8_t)(rec.crc32 >> 24);
            data[21] = (uint8_t)(rec.crc32 >> 16);
            data[22] = (uint8_t)(rec.crc32 >> 8);
            data[23] = (uint8_t)(rec.crc32);

            handle_response(STATUS_OK, cmd, param_addr, data, sizeof(data), use_uart1);
            break;
        }

        case 0x0814: /* READ: Get log count */
        {
            uint32_t cnt;
            uint8_t data[4];

            if (!flash_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            cnt = flash_log_count();

            data[0] = (uint8_t)(cnt >> 24);
            data[1] = (uint8_t)(cnt >> 16);
            data[2] = (uint8_t)(cnt >> 8);
            data[3] = (uint8_t)(cnt);

            handle_response(STATUS_OK, cmd, param_addr, data, 4, use_uart1);
            break;
        }

        case 0x0815: /* SERVICE: Clear whole log area */
        {
            if (!flash_present) {
                uint8_t err = 1;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            if (flash_log_clear() != 1) {
                uint8_t err = 2;
                handle_response(ERROR_RESPONSE, cmd, param_addr, &err, 1, use_uart1);
                break;
            }

            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            break;
        }

        case 0x9999: /*SERVICE: Reset device */
        {
            handle_response(STATUS_OK, cmd, param_addr, NULL, 0, use_uart1);
            systick_delay_ms(100);
            NVIC_SystemReset();
            break;
        }

        default:
            handle_response(ERROR_RESPONSE, cmd, param_addr, NULL, 0, use_uart1);
            break;
    }
}

void uart2_process_rx(void)
{
    uint16_t pos = (uint16_t)(UART2_RX_BUFFER_SIZE - DMA1_Stream5->NDTR);

    while (uart2_rx_old_pos != pos) {
        uint8_t b = uart2_rx_buf[uart2_rx_old_pos++];
        if (uart2_rx_old_pos >= UART2_RX_BUFFER_SIZE) uart2_rx_old_pos = 0;

        if (uart2_frame_idx == 0 && b != DEV_ADDR) continue;

        uart2_frame_acc[uart2_frame_idx++] = b;
        if (uart2_frame_idx == UART2_RX_FRAME_LEN) {
            if (crc8_atm(uart2_frame_acc, FRAME_LEN_APP - 1) == uart2_frame_acc[FRAME_LEN_APP - 1]) {
                handle_request(uart2_frame_acc, 0);
            }
            uart2_frame_idx = 0;
            if (b == DEV_ADDR) {
                uart2_frame_acc[0] = b;
                uart2_frame_idx = 1;
            }
        }
    }
}

void uart1_process_rx(void)
{
    uint16_t pos = (uint16_t)(UART1_RX_BUFFER_SIZE - DMA2_Stream5->NDTR);

    while (uart1_rx_old_pos != pos) {
        uint8_t b = uart1_rx_buf[uart1_rx_old_pos++];
        if (uart1_rx_old_pos >= UART1_RX_BUFFER_SIZE) uart1_rx_old_pos = 0;

        if (uart1_frame_idx == 0 && b != DEV_ADDR) continue;

        uart1_frame_acc[uart1_frame_idx++] = b;
        if (uart1_frame_idx == UART1_RX_FRAME_LEN) {
            if (crc8_atm(uart1_frame_acc, FRAME_LEN_APP - 1) == uart1_frame_acc[FRAME_LEN_APP - 1]) {
                handle_request(uart1_frame_acc, 1);
            }
            uart1_frame_idx = 0;
            if (b == DEV_ADDR) {
                uart1_frame_acc[0] = b;
                uart1_frame_idx = 1;
            }
        }
    }
}