#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "stm32f4xx.h"

typedef struct
{
	uint32_t std_id;
	uint32_t ext_id;
	uint32_t ide;
	uint32_t rtr;
	uint32_t dlc;
	uint8_t transmit_global_time;

}can_tx_header_typedef;


typedef struct
{
  uint32_t std_id;
  uint32_t ext_id;
  uint32_t ide;
  uint32_t rtr;
  uint32_t dlc;
  uint32_t timestamp;
  uint32_t filter_match_index;

} can_rx_header_typedef;

extern volatile uint8_t can_last_error;

#define CAN_MODE_LOOPBACK  0x00
#define CAN_MODE_NORMAL  0x01

#define CAN_ID_STD		0x00
#define CAN_ID_EXT		0x04

#define CAN_RX_FIFO0	0x00
#define CAN_RX_FIFO1	0x01

extern volatile uint32_t can_diag;

void can_gpio_init(void);
void can_params_init(uint8_t mode);
void can_start(void);
uint8_t can_add_tx_message(can_tx_header_typedef *pHeader, uint8_t aData[], uint32_t *pTxMailbox);
uint8_t can_get_rx_message(uint32_t RxFifo, can_rx_header_typedef *pHeader, uint8_t aData[]);
void can_filter_config(uint16_t std_id);
void can_filter_accept_all(void);

#endif // CAN_H