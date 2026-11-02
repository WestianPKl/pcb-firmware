
#ifndef NRF_H
#define NRF_H

#include <stdint.h>

#define NRF_CE_PIN  8U // PF8
#define NRF_IRQ_PIN 7U // PF7

#define NUM_REGISTERS   26U

#define REG_CONFIG      0x00U
#define REG_EN_AA       0x01U
#define REG_EN_RXADDR   0x02U
#define REG_SETUP_AW    0x03U
#define REG_SETUP_RETR  0x04U
#define REG_RF_CH       0x05U
#define REG_RF_SETUP    0x06U
#define REG_STATUS      0x07U
#define REG_OBSERVE_TX  0x08U
#define REG_RPD         0x09U
#define REG_RX_ADDR_P0  0x0AU
#define REG_RX_ADDR_P1  0x0BU
#define REG_RX_ADDR_P2  0x0CU
#define REG_RX_ADDR_P3  0x0DU
#define REG_RX_ADDR_P4  0x0EU
#define REG_RX_ADDR_P5  0x0FU
#define REG_TX_ADDR     0x10U
#define REG_RX_PW_P0    0x11U
#define REG_RX_PW_P1    0x12U
#define REG_RX_PW_P2    0x13U
#define REG_RX_PW_P3    0x14U
#define REG_RX_PW_P4    0x15U
#define REG_RX_PW_P5    0x16U
#define REG_FIFO_STATUS 0x17U
#define REG_DYNPD       0x1CU
#define REG_FEATURE     0x1DU

#define CONFIG_MASK_RX_DR  6U
#define CONFIG_MASK_TX_DS  5U
#define CONFIG_MASK_MAX_RT 4U
#define CONFIG_EN_CRC      3U
#define CONFIG_CRCO        2U
#define CONFIG_PWR_UP      1U
#define CONFIG_PRIM_RX     0U

#define STATUS_RX_DR       6U
#define STATUS_TX_DS       5U
#define STATUS_MAX_RT      4U
#define STATUS_RX_P_NO     1U
#define STATUS_TX_FULL     0U

#define FIFO_STATUS_TX_REUSE      6U
#define FIFO_STATUS_TX_FULL       5U
#define FIFO_STATUS_TX_FIFO_EMPTY 4U
#define FIFO_STATUS_RX_FULL       1U
#define FIFO_STATUS_RX_FIFO_EMPTY 0U

#define FEATURE_EN_DPL        2U
#define FEATURE_EN_ACK_PAY    1U
#define FEATURE_EN_DYN_ACK    0U

#define CMD_R_REGISTER      0x00U
#define CMD_W_REGISTER      0x20U
#define CMD_ACTIVATE        0x50U
#define CMD_R_RX_PL_WID     0x60U
#define CMD_R_RX_PAYLOAD    0x61U
#define CMD_W_TX_PAYLOAD    0xA0U
#define CMD_W_ACK_PAYLOAD   0xA8U
#define CMD_W_TX_PAYLOAD_NO_ACK  0xB0U
#define CMD_FLUSH_TX        0xE1U
#define CMD_FLUSH_RX        0xE2U
#define CMD_REUSE_TX_PL     0xE3U
#define CMD_NOP             0xFFU

#define NRF24_PAYLOAD_SIZE 8U
#define NRF24_ADDR_SIZE	3U

#define NRF_EVENT_NONE       0x00U
#define NRF_EVENT_RX_DATA    0x01U
#define NRF_EVENT_TX_OK      0x02U
#define NRF_EVENT_TX_FAILED  0x04U
#define NRF_EVENT_ERROR      0x80U

extern volatile uint8_t nrf_irq;

uint8_t nrf_init(void);
uint8_t nrf_receive_data(uint8_t *data, uint16_t length);
uint8_t nrf_send_data(const uint8_t *data, uint16_t length);
uint8_t nrf_read_data(uint8_t *data, uint16_t length);
uint8_t nrf_process_irq(uint8_t *rx_data);
void nrf_handle_tx_status(void);
void nrf_flush_rx(void);

#endif // NRF_H
