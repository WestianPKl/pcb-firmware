#ifndef UART_PROTOCOL_H
#define UART_PROTOCOL_H

#include <stdint.h>

#define FRAME_LEN_APP   32
#define FRAME_PAYLOAD   (FRAME_LEN_APP - 5)
#define UART2_RX_BUFFER_SIZE 128
#define UART2_RX_FRAME_LEN FRAME_LEN_APP
#define UART1_RX_BUFFER_SIZE 128
#define UART1_RX_FRAME_LEN FRAME_LEN_APP

extern uint8_t uart2_rx_buf[UART2_RX_BUFFER_SIZE];
extern uint8_t uart1_rx_buf[UART1_RX_BUFFER_SIZE];

extern uint8_t uart2_tx_frame[FRAME_LEN_APP];
extern uint8_t uart1_tx_frame[FRAME_LEN_APP];

extern volatile uint8_t uart2_tx_busy;
extern volatile uint8_t uart1_tx_busy;

void uart2_process_rx(void);
void uart1_process_rx(void);

#endif // UART_PROTOCOL_H