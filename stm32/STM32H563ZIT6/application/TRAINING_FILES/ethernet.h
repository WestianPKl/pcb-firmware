#ifndef ETHERNET_H
#define ETHERNET_H

#include <stdint.h>
#include "stm32h5xx.h"

#define RMII_REF_CLK_Pin 1U // PA1
#define RMII_MDIO_Pin 2U // PA2
#define RMII_MDC_Pin 1U // PC1
#define RMII_CRS_DV_Pin 7U // PA7
#define RMII_RXD0_Pin 4U // PC4
#define RMII_RXD1_Pin 5U // PC5
#define RMII_TX_EN_Pin 11U // PG11
#define RMII_TXD0_Pin 13U // PG13
#define RMII_TXD1_Pin 15U // PB15

#define BMCR 0x00U // Basic Control Register
#define BMSR 0x01U // Basic Status Register
#define PHIDR1 0x02U // PHY Identifier 1 Register
#define PHIDR2 0x03U // PHY Identifier 2 Register
#define ANAR 0x04U // Auto-Negotiation Advertisement Register
#define ANLPAR 0x05U // Auto-Negotiation Link Partner Ability Register
#define ANER 0x06U // Auto-Negotiation Expansion Register
#define ANNPTR 0x07U // Auto Negotiation Next Page TX Register
#define ANNPDR 0x08U // Auto Negotiation Next Page RX Register
#define MMD_CTRL 0x0DU // MMD Access Control Register
#define MMD_DATA 0x0EU // MMD Access Address/Data Register
#define EDPD_NLP 0x10U // EDPD Register
#define MDSR 0x11U // Mode Control/Status Register
#define SMR 0x12U // Special Modes Register
#define TDR_PDC 0x14U // TDR Patterns/Delay Control Register
#define TDR_CSR 0x15U // TDR Control/Status Register
#define SEC 0x1AU // Symbol Error Counter Register
#define SCSI 0x1BU // Special Control/Status Indications Register
#define CLR 0x1CU // Cable Length Register
#define ISFR 0x1DU // Interrupt Source Flag Register
#define IMR 0x1EU // Interrupt Mask Register
#define PHY_CTRL 0x1FU   // PHY Special Control/Status Register

#define ETH_TYPE_ARP       0x0806U
#define ARP_HTYPE_ETH      0x0001U
#define ARP_PTYPE_IPV4     0x0800U
#define ARP_REQUEST        0x0001U
#define ARP_REPLY          0x0002U
#define ETH_MIN_FRAME_SIZE 60U


#define ARP_HTYPE_OFFSET       14U
#define ARP_PTYPE_OFFSET       16U
#define ARP_HLEN_OFFSET        18U
#define ARP_PLEN_OFFSET        19U
#define ARP_OPCODE_OFFSET      20U
#define ARP_SENDER_MAC_OFFSET  22U
#define ARP_SENDER_IP_OFFSET   28U
#define ARP_TARGET_MAC_OFFSET  32U
#define ARP_TARGET_IP_OFFSET   38U

#define ETH_TYPE_IPV4     0x0800U
#define IP_PROTO_ICMP     0x01U

#define ICMP_ECHO_REPLY   0U
#define ICMP_ECHO_REQUEST 8U

typedef struct
{
    volatile uint32_t DESC0;
    volatile uint32_t DESC1;
    volatile uint32_t DESC2;
    volatile uint32_t DESC3;
} eth_dma_desc_t;

uint8_t ethernet_init(void);
uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr);
uint8_t ethernet_send_test_frame(void);
uint8_t ethernet_rx_poll(uint8_t **frame, uint16_t *length);
void ethernet_rx_release(void);
void ethernet_process_frame(uint8_t *frame, uint16_t length);

#endif // ETHERNET_H
