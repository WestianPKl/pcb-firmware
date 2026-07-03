#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include "stm32f7xx.h"

extern volatile uint8_t eth_rx_ready;
extern volatile uint32_t eth_isr_count;

#define PHY_ADDR          0x00U
#define PHY_BCR           0x00U
#define PHY_BSR           0x01U
#define PHY_BCR_RESET     (1U << 15)
#define PHY_BCR_AN_RESTART (1U << 9)
#define PHY_BCR_AN_ENABLE (1U << 12)
#define PHY_BSR_LINK_UP   (1U << 2)

#define MACMIIAR_CR       (0x4U << 2)

#define TDES0_OWN  (1U << 31)
#define TDES0_LS   (1U << 29)
#define TDES0_FS   (1U << 28)
#define TDES0_CIC  (3U << 22)
#define TDES0_TCH  (1U << 20)
#define RDES0_OWN  (1U << 31)
#define RDES0_ES   (1U << 15)
#define RDES1_RCH  (1U << 14)
#define RDES1_RBS1 (0x1FFFU)

#define ETH_RX_DESC_CNT 4U
#define ETH_TX_DESC_CNT 4U
#define ETH_BUF_SIZE    1524U

void ethernet_init(void);

uint8_t eth_send_frame(const uint8_t *data, uint16_t len);
uint16_t eth_recv_frame(uint8_t *buf, uint16_t maxlen);
uint8_t  eth_link_up(void);

#endif // ETHERNET_H
