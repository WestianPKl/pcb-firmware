#include "protocol.h"
#include "support.h"
#include "can.h"
#include <stdio.h>

#define STATUS_OK       0x40
#define ERROR_RESPONSE  0x7F

uint32_t tx_mailbox;
volatile uint8_t counter;

volatile can_rx_header_typedef can_rx_header_last;
volatile uint8_t can_rx_data_last[8];
volatile uint8_t can_rx_ready;

uint16_t protocol_process_request(const protocol_request_t *request, uint8_t *frame)
{
    uint8_t length = 0U;
    uint8_t status;

    frame[0] = MAGIC;
    frame[2] = ADDRESS;
    frame[3] = request->sequence;
    frame[4] = request->command;

    switch (request->command) {
        case 0x00:
            frame[6] = 0x11;
            frame[7] = 0x22;
            frame[8] = 0x33;
            length = 3U;
            status = PROTOCOL_STATUS_OK;
            break;
        default:
            status = PROTOCOL_STATUS_ERROR;
            break;
    }

    frame[1] = length;
    frame[5] = status;

    uint16_t crc = crc16_ccitt(&frame[1], 5U + length);
    frame[6U + length] = (uint8_t)(crc >> 8);
    frame[7U + length] = (uint8_t)(crc & 0xFFU);

    return (uint16_t)(8U + length);
}

int send_frame_0x10(void)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x10;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 5;
    tx_header.transmit_global_time = 0;

    tx_data[0] = 0x10;
    tx_data[1] = 0x20;
    tx_data[2] = 0x30;
    tx_data[3] = 0x40;
    tx_data[4] = counter++;

    if (counter > 250) counter = 0;

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

int send_frame_0x30(const bme280_data_t *bme280_data)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x30;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 7;
    tx_header.transmit_global_time = 0;

    tx_data[0] = (uint8_t)((bme280_data->temperature >> 8) & 0xFF);
    tx_data[1] = (uint8_t)(bme280_data->temperature & 0xFF);
    tx_data[2] = (uint8_t)((bme280_data->humidity >> 8) & 0xFF);
    tx_data[3] = (uint8_t)(bme280_data->humidity & 0xFF);
    tx_data[4] = (uint8_t)((bme280_data->pressure >> 16) & 0xFF);
    tx_data[5] = (uint8_t)((bme280_data->pressure >> 8) & 0xFF);
    tx_data[6] = (uint8_t)(bme280_data->pressure & 0xFF);

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);

}

int send_frame_0x31(const sht40_data_t *sht40_data, const uint16_t *adc_data)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x31;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 6;
    tx_header.transmit_global_time = 0;

    tx_data[0] = (uint8_t)((sht40_data->temperature >> 8) & 0xFF);
    tx_data[1] = (uint8_t)(sht40_data->temperature & 0xFF);
    tx_data[2] = (uint8_t)((sht40_data->humidity >> 8) & 0xFF);
    tx_data[3] = (uint8_t)(sht40_data->humidity & 0xFF);
    tx_data[4] = (uint8_t)((adc_data[0] >> 8) & 0xFF);
    tx_data[5] = (uint8_t)(adc_data[0] & 0xFF);

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}

int send_frame_0x32(const rtc_datetime_t *rtc_datetime)
{
    can_tx_header_typedef tx_header;
    uint8_t tx_data[8] = {0};

    tx_header.std_id = 0x32;
    tx_header.ext_id = 0;
    tx_header.ide = CAN_ID_STD;
    tx_header.rtr = 0;
    tx_header.dlc = 8;
    tx_header.transmit_global_time = 0;

    tx_data[0] = rtc_datetime->seconds;
    tx_data[1] = rtc_datetime->minutes;
    tx_data[2] = rtc_datetime->hours;
    tx_data[3] = rtc_datetime->weekday;
    tx_data[4] = rtc_datetime->day;
    tx_data[5] = rtc_datetime->month;
    tx_data[6] = (uint8_t)((rtc_datetime->year >> 8) & 0xFF);
    tx_data[7] = (uint8_t)(rtc_datetime->year & 0xFF);

    return can_add_tx_message(&tx_header, tx_data, &tx_mailbox);
}