#include "can.h"
#include "can_protocol.h"
#include <stdint.h>
#include <string.h>
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

#define STATUS_OK       0x40
#define ERROR_RESPONSE  0x7F

uint32_t tx_mailbox;
volatile uint8_t counter;

volatile can_rx_header_typedef can_rx_header_last;
volatile uint8_t can_rx_data_last[8];
volatile uint8_t can_rx_ready;

static int send_frame_0x10(void)
{
    const device_info_t *info = device_info_get();
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x10;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 5;
    tx_header.transmit_global_time = 0;

    uint32_t serial = 0;
    if (info->magic == INFO_MAGIC) serial = info->serial;
    uint8_t serial_bytes[4] = {
                (uint8_t)((serial >> 24) & 0xFF),
                (uint8_t)((serial >> 16) & 0xFF),
                (uint8_t)((serial >> 8) & 0xFF),
                (uint8_t)(serial & 0xFF)
            };

    tx_data[0] = serial_bytes[0];
    tx_data[1] = serial_bytes[1];
    tx_data[2] = serial_bytes[2];
    tx_data[3] = serial_bytes[3];
    tx_data[4] = counter++;

    if (counter > 250) counter = 0;

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x11(void)
{
    const device_info_t *info = device_info_get();
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x11;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 5;
    tx_header.transmit_global_time = 0;

    uint8_t hwmaj = 0, hwmin = 0;
    if (info->magic == INFO_MAGIC) { hwmaj = info->hw_major;
        hwmin = info->hw_minor; }
    uint8_t payload[5] = {
        FW_VERSION_MAJOR, FW_VERSION_MINOR, FW_VERSION_PATCH, hwmaj, hwmin
    };

    tx_data[0] = payload[0];
    tx_data[1] = payload[1];
    tx_data[2] = payload[2];
    tx_data[3] = payload[3];
    tx_data[4] = payload[4];

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x12(void)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x12;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 8;
    tx_header.transmit_global_time = 0;
    uint8_t build_date[10] = {0};
    memcpy(build_date, FW_BUILD_DATE, 10);

    tx_data[0] = build_date[0];
    tx_data[1] = build_date[1];
    tx_data[2] = build_date[2];
    tx_data[3] = build_date[3];
    tx_data[4] = build_date[5];
    tx_data[5] = build_date[6];
    tx_data[6] = build_date[8];
    tx_data[7] = build_date[9];

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x13(void)
{
    const device_info_t *info = device_info_get();
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x13;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 8;
    tx_header.transmit_global_time = 0;

    uint8_t production_date[10] = {0};
    if (info->magic == INFO_MAGIC) memcpy(production_date, info->prod_date, 10);

    tx_data[0] = production_date[0];
    tx_data[1] = production_date[1];
    tx_data[2] = production_date[2];
    tx_data[3] = production_date[3];
    tx_data[4] = production_date[5];
    tx_data[5] = production_date[6];
    tx_data[6] = production_date[8];
    tx_data[7] = production_date[9];

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x20(void){
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};
    
    tx_header.std_id = 0x20;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 4;
    tx_header.transmit_global_time = 0;
    
    if (!adc_present) {
        uint8_t err = 1;

        tx_data[0] = ERROR_RESPONSE;
        tx_data[1] = err;

        return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
    }

    if (ADC_BUFFER_SIZE < 2) {
        uint8_t err = 2;

        tx_data[0] = ERROR_RESPONSE;
        tx_data[1] = err;

        return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
    }

    uint16_t v0 = adc_data_buffer[0];
    uint16_t v1 = adc_data_buffer[1];
    uint8_t data[4] = {(uint8_t)(v0 >> 8), (uint8_t)v0, (uint8_t)(v1 >> 8), (uint8_t)v1};

    tx_data[0] = data[0];
    tx_data[1] = data[1];
    tx_data[2] = data[2];
    tx_data[3] = data[3];
    
    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x30(void)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x30;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 4;
    tx_header.transmit_global_time = 0;

    if (!sht40_present) {
        tx_data[0] = ERROR_RESPONSE;
        tx_data[1] = 1U;
        return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
    }

    if (sht40_error_flag) {
        tx_data[0] = ERROR_RESPONSE;
        tx_data[1] = 2U;
        return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
    }

    int16_t temp_c_x100 = measurement_sht40.temperature;
    uint16_t rh_x100 = measurement_sht40.humidity;

    tx_data[0] = (uint8_t)((temp_c_x100 >> 8) & 0xFF);
    tx_data[1] = (uint8_t)(temp_c_x100 & 0xFF);
    tx_data[2] = (uint8_t)((rh_x100 >> 8) & 0xFF);
    tx_data[3] = (uint8_t)(rh_x100 & 0xFF);

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x31(void)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x31;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 8;
    tx_header.transmit_global_time = 0;

    if (!bme280_present) {
        tx_data[0] = ERROR_RESPONSE;
        tx_data[1] = 1U;
        return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
    }

    if (bme280_error_flag) {
        tx_data[0] = ERROR_RESPONSE;
        tx_data[1] = 2U;
        return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
    }

    int16_t temp_x100   = (int16_t)measurement_bme280.temperature;
    uint16_t hum_x100   = (uint16_t)measurement_bme280.humidity;
    uint32_t press_pa   = measurement_bme280.pressure;

    tx_data[0] = (uint8_t)((temp_x100 >> 8) & 0xFF);
    tx_data[1] = (uint8_t)(temp_x100 & 0xFF);
    tx_data[2] = (uint8_t)((hum_x100 >> 8) & 0xFF);
    tx_data[3] = (uint8_t)(hum_x100 & 0xFF);
    tx_data[4] = (uint8_t)((press_pa >> 24) & 0xFF);
    tx_data[5] = (uint8_t)((press_pa >> 16) & 0xFF);
    tx_data[6] = (uint8_t)((press_pa >> 8) & 0xFF);
    tx_data[7] = (uint8_t)(press_pa & 0xFF);

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

static int send_frame_0x60(void)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x60;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 8;
    tx_header.transmit_global_time = 0;

    tx_data[0] = datetime.year;
    tx_data[1] = datetime.month;
    tx_data[2] = datetime.day;
    tx_data[3] = datetime.weekday;
    tx_data[4] = datetime.hours;
    tx_data[5] = datetime.minutes;
    tx_data[6] = datetime.seconds;

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

void send_cyclic_frames(void)
{
    static uint8_t slot = 0;

    switch (slot) {
        case 0:
            send_frame_0x10();
            break;
        case 1:
            send_frame_0x11();
            break;
        case 2:
            send_frame_0x12();
            break;
        case 3:
            send_frame_0x13();
            break;
        case 4:
            send_frame_0x20();
            break;
        case 5:
            send_frame_0x30();
            break;
        case 6:
            send_frame_0x31();
            break;
        case 7:
            send_frame_0x60();
            break;
        default:
            break;
    }

    slot++;
    if (slot >= 8) {
        slot = 0;
    }
}

// void prepare_answers(void){

// }

// void handle_can_message(uint8_t cmd, uint8_t param_addr, const uint8_t *payload, uint32_t payload_len)
// {
//     uint16_t cmd_combined = ((uint16_t)cmd << 8) | param_addr;
    
//     can_tx_header_typedef tx_header;
//     uint8_t tx_data[8] = {0};

//     tx_header.std_id = 0x385;
//     tx_header.ext_id = 0;
//     tx_header.ide = CAN_ID_STD;
//     tx_header.rtr = 0;
//     tx_header.dlc = 8;
//     tx_header.transmit_global_time = 0;

//     switch (cmd_combined) {
//         case 0x0401: /*OPERATIONAL: Set LED1 state (1 byte: 0 or 1) */
//             uint8_t val = payload[0] ? 1U : 0U;

//             if (val) pin_set_high('B', 14U);
//             else     pin_set_low('B', 14U);
            
//             led1_state = val;
//             tx_data[0] = STATUS_OK;
//             tx_data[1] = led1_state;

//             return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
//             break;
//         default:
//             tx_data[0] = ERROR_RESPONSE;
//             break;
//     }
// }