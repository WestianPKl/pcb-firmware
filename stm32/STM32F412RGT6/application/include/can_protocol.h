#ifndef CAN_PROTOCOL_H
#define CAN_PROTOCOL_H

#include <stdint.h>

extern volatile can_rx_header_typedef can_rx_header_last;
extern volatile uint8_t can_rx_data_last[8];
extern volatile uint8_t can_rx_ready;

void send_cyclic_frames(void);

#endif // CAN_PROTOCOL_H