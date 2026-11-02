#include "ethernet.h"

#define TDES3_OWN      (1UL << 31U)
#define TDES3_FD       (1UL << 29U)
#define TDES3_LD       (1UL << 28U)
#define TDES3_CIC_Msk  (0x3UL << 16U)
#define TDES3_FL_Msk   (0x7FFFUL << 0U)
#define TDES2_B1L_Msk  (0x3FFFUL << 0U)
#define RDES3_OWN  (1UL << 31U)
#define RDES3_FD   (1UL << 29U)
#define RDES3_LD   (1UL << 28U)
#define RDES3_BUF1V  (1UL << 24U)
#define RDES3_ES   (1UL << 15U)
#define RDES3_PL_Msk  (0x7FFFUL << 0U)


#define ETH_RX_DESC_COUNT 4U
#define ETH_BUF_SIZE    1536U

volatile uint8_t arp_reply_status = 0U;

static eth_dma_desc_t tx_desc __attribute__((aligned(32)));
static uint8_t tx_buffer[ETH_BUF_SIZE] __attribute__((aligned(32)));

static uint32_t rx_index = 0U;
static eth_dma_desc_t rx_desc[ETH_RX_DESC_COUNT]
    __attribute__((aligned(32)));
static uint8_t rx_buffer[ETH_RX_DESC_COUNT][ETH_BUF_SIZE]
    __attribute__((aligned(32)));


static const uint8_t eth_mac[6] =
{
    0x02U, 0x00U, 0x00U, 0x00U, 0x00U, 0x01U
};

static const uint8_t eth_ip[4] =
{
    192U, 168U, 18U, 200U
};

#define ETH_TEST_FRAME_SIZE 60U

static uint16_t eth_build_test_frame(void)
{
    tx_buffer[0]  = 0xFFU;
    tx_buffer[1]  = 0xFFU;
    tx_buffer[2]  = 0xFFU;
    tx_buffer[3]  = 0xFFU;
    tx_buffer[4]  = 0xFFU;
    tx_buffer[5]  = 0xFFU;

    tx_buffer[6]  = 0x02U;
    tx_buffer[7]  = 0x00U;
    tx_buffer[8]  = 0x00U;
    tx_buffer[9]  = 0x00U;
    tx_buffer[10] = 0x00U;
    tx_buffer[11] = 0x01U;

    tx_buffer[12] = 0x88U;
    tx_buffer[13] = 0xB5U;

    for (uint32_t i = 14U; i < ETH_TEST_FRAME_SIZE; i++)
    {
        tx_buffer[i] = (uint8_t)(i - 14U);
    }

    return ETH_TEST_FRAME_SIZE;
}


static void eth_pin_af11(GPIO_TypeDef *port, uint32_t pin)
{
    port->MODER &= ~(0x3U << (pin * 2U));
    port->MODER |=  (0x2U << (pin * 2U));
    port->OTYPER &= ~(0x1U << pin);
    port->OSPEEDR &= ~(0x3U << (pin * 2U));
    port->OSPEEDR |=  (0x3U << (pin * 2U));
    port->PUPDR &= ~(0x3U << (pin * 2U));

    if (pin < 8U)
    {
        port->AFR[0] &= ~(0xFU << (pin * 4U));
        port->AFR[0] |=  (0xBU << (pin * 4U));
    }
    else
    {
        port->AFR[1] &= ~(0xFU << ((pin - 8U) * 4U));
        port->AFR[1] |=  (0xBU << ((pin - 8U) * 4U));
    }
}



static uint8_t ethernet_tx_start(uint16_t len)
{
    if ((tx_desc.DESC3 & TDES3_OWN) != 0U)
    {
        return 0U;
    }

    if ((len < 14U) || (len > ETH_BUF_SIZE))
    {
        return 0U;
    }

    tx_desc.DESC0 = (uint32_t)tx_buffer;
    tx_desc.DESC1 = 0U;
    tx_desc.DESC2 = len & TDES2_B1L_Msk;

    tx_desc.DESC3 = TDES3_FD | TDES3_LD | (len & TDES3_FL_Msk);

    __DSB();

    tx_desc.DESC3 |= TDES3_OWN;

    __DSB();

    ETH->DMACTDTPR = (uint32_t)&tx_desc;

    return 1U;
}

static uint16_t internet_checksum(const uint8_t *data, uint16_t length)
{
    uint32_t sum = 0U;

    while (length > 1U)
    {
        sum += ((uint16_t)data[0] << 8U) |
                (uint16_t)data[1];

        data += 2;
        length -= 2U;
    }

    if (length != 0U)
    {
        sum += ((uint16_t)data[0] << 8U);
    }

    while ((sum >> 16U) != 0U)
    {
        sum = (sum & 0xFFFFU) + (sum >> 16U);
    }

    return (uint16_t)(~sum);
}

static uint8_t ethernet_handle_ipv4(const uint8_t *frame, uint16_t length)
{
    if (length < 34U)
    {
        return 0U;
    }

    /* Ethernet header = 14 B */
    const uint8_t *ip = &frame[14];

    /* IPv4? */
    if ((ip[0] >> 4U) != 4U)
    {
        return 0U;
    }

    uint8_t ihl = (ip[0] & 0x0FU) * 4U;

    if (ihl < 20U)
    {
        return 0U;
    }

    uint16_t ip_total_length =
        ((uint16_t)ip[2] << 8U) |
         (uint16_t)ip[3];

    if (ip_total_length < ihl)
    {
        return 0U;
    }

    if ((14U + ip_total_length) > length)
    {
        return 0U;
    }

    /* Destination IP musi być nasze */
    if ((ip[16] != eth_ip[0]) ||
        (ip[17] != eth_ip[1]) ||
        (ip[18] != eth_ip[2]) ||
        (ip[19] != eth_ip[3]))
    {
        return 0U;
    }

    /* Protocol = ICMP */
    if (ip[9] != IP_PROTO_ICMP)
    {
        return 0U;
    }

    uint8_t *icmp = (uint8_t *)&frame[14U + ihl];

    uint16_t icmp_length = ip_total_length - ihl;

    if (icmp_length < 8U)
    {
        return 0U;
    }

    /* Echo Request? */
    if ((icmp[0] != ICMP_ECHO_REQUEST) ||
        (icmp[1] != 0U))
    {
        return 0U;
    }

    /*
     * Kopiujemy Ethernet + cały pakiet IPv4.
     * Nie kopiujemy FCS z RX.
     */
    uint16_t tx_length = 14U + ip_total_length;

    for (uint16_t i = 0U; i < tx_length; i++)
    {
        tx_buffer[i] = frame[i];
    }

    /*
     * Ethernet:
     * DST reply = SRC requestu
     */
    for (uint32_t i = 0U; i < 6U; i++)
    {
        tx_buffer[i] = frame[6U + i];
    }

    /* SRC = nasz MAC */
    for (uint32_t i = 0U; i < 6U; i++)
    {
        tx_buffer[6U + i] = eth_mac[i];
    }

    uint8_t *tx_ip = &tx_buffer[14];

    /*
     * IPv4:
     * SRC = nasze IP
     * DST = IP komputera z requestu
     */
    for (uint32_t i = 0U; i < 4U; i++)
    {
        tx_ip[16U + i] = ip[12U + i];
        tx_ip[12U + i] = eth_ip[i];
    }

    /* typowy TTL odpowiedzi */
    tx_ip[8] = 64U;

    /*
     * Przelicz IPv4 header checksum.
     */
    tx_ip[10] = 0U;
    tx_ip[11] = 0U;

    uint16_t ip_checksum = internet_checksum(tx_ip, ihl);

    tx_ip[10] = (uint8_t)(ip_checksum >> 8U);
    tx_ip[11] = (uint8_t)(ip_checksum & 0xFFU);

    /*
     * ICMP zaczyna się po IPv4 header.
     */
    uint8_t *tx_icmp = &tx_buffer[14U + ihl];

    /*
     * Echo Request = type 8
     * Echo Reply   = type 0
     *
     * ID, sequence i payload zostawiamy bez zmian.
     */
    tx_icmp[0] = ICMP_ECHO_REPLY;
    tx_icmp[1] = 0U;

    tx_icmp[2] = 0U;
    tx_icmp[3] = 0U;

    uint16_t icmp_checksum =
        internet_checksum(tx_icmp, icmp_length);

    tx_icmp[2] = (uint8_t)(icmp_checksum >> 8U);
    tx_icmp[3] = (uint8_t)(icmp_checksum & 0xFFU);

    return ethernet_tx_start(tx_length);
}

uint8_t ethernet_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN | RCC_AHB2ENR_GPIOCEN | RCC_AHB2ENR_GPIOGEN;
    (void)RCC->AHB2ENR;

    eth_pin_af11(GPIOA, RMII_REF_CLK_Pin);
    eth_pin_af11(GPIOA, RMII_MDIO_Pin);
    eth_pin_af11(GPIOC, RMII_MDC_Pin);
    eth_pin_af11(GPIOA, RMII_CRS_DV_Pin);
    eth_pin_af11(GPIOC, RMII_RXD0_Pin);
    eth_pin_af11(GPIOC, RMII_RXD1_Pin);
    eth_pin_af11(GPIOG, RMII_TX_EN_Pin);
    eth_pin_af11(GPIOG, RMII_TXD0_Pin);
    eth_pin_af11(GPIOB, RMII_TXD1_Pin);

    RCC->AHB1ENR |= RCC_AHB1ENR_ETHEN | RCC_AHB1ENR_ETHTXEN | RCC_AHB1ENR_ETHRXEN;
    (void)RCC->AHB1ENR;

    RCC->APB3ENR |= RCC_APB3ENR_SBSEN;
    (void)RCC->APB3ENR;

    SBS->PMCR &= ~SBS_PMCR_ETH_SEL_PHY_Msk;
    SBS->PMCR |=  (0x4U << SBS_PMCR_ETH_SEL_PHY_Pos);
    (void)SBS->PMCR;

    ETH->DMAMR |= ETH_DMAMR_SWR;
    while ((ETH->DMAMR & ETH_DMAMR_SWR) != 0U){}

    ETH->MACMDIOAR &= ~ETH_MACMDIOAR_CR_Msk;
    ETH->MACMDIOAR |= (0x4U << ETH_MACMDIOAR_CR_Pos);

    uint32_t timeout = 1000000U;
    uint16_t phy_ctrl = 0U;

    while (timeout > 0U)
    {
        phy_ctrl = eth_phy_read(0U, 31U);

        if ((phy_ctrl & (1U << 12U)) != 0U)
        {
            break;
        }

        timeout--;
    }

    if (timeout == 0U)
    {
        return 0U;
    }

    uint8_t speed = (phy_ctrl >> 2U) & 0x7U;

    ETH->MACCR &= ~(ETH_MACCR_DM | ETH_MACCR_FES);

    if (speed == 5U)
    {
        /* 10 Mbps Full Duplex */
        ETH->MACCR |= ETH_MACCR_DM;
    }
    else if (speed == 2U)
    {
        /* 100 Mbps Half Duplex */
        ETH->MACCR |= ETH_MACCR_FES;
    }
    else if (speed == 6U)
    {
        /* 100 Mbps Full Duplex */
        ETH->MACCR |= ETH_MACCR_FES | ETH_MACCR_DM;
    }
    else if (speed == 1U)
    {
        /* 10 Mbps Half Duplex */
    }
    else
    {
        return 0U;
    }

    ETH->MACA0LR = ((uint32_t)eth_mac[3] << 24U) | ((uint32_t)eth_mac[2] << 16U)| ((uint32_t)eth_mac[1] <<  8U) | ((uint32_t)eth_mac[0]);
    ETH->MACA0HR = ((uint32_t)eth_mac[5] << 8U) | ((uint32_t)eth_mac[4]);

    /* MACPFR reset value = 0 is suitable for now */

    ETH->MTLTQOMR |= ETH_MTLTQOMR_TSF;
    ETH->MTLRQOMR |= ETH_MTLRQOMR_RSF;

    tx_desc.DESC0 = 0U;
    tx_desc.DESC1 = 0U;
    tx_desc.DESC2 = 0U;
    tx_desc.DESC3 = 0U;

    ETH->DMACTDLAR = (uint32_t)&tx_desc;
    ETH->DMACTDRLR = 0U;

    for (uint32_t i = 0U; i < ETH_RX_DESC_COUNT; i++)
    {
        rx_desc[i].DESC0 = (uint32_t)rx_buffer[i];
        rx_desc[i].DESC1 = 0U;
        rx_desc[i].DESC2 = 0U;
        rx_desc[i].DESC3 = RDES3_BUF1V;

        __DSB();

        rx_desc[i].DESC3 |= RDES3_OWN;
    }

    __DSB();

    ETH->DMACRDLAR = (uint32_t)&rx_desc[0];
    ETH->DMACRDRLR = ETH_RX_DESC_COUNT - 1U;
    ETH->DMACRDTPR = (uint32_t)&rx_desc[ETH_RX_DESC_COUNT - 1U];

    ETH->DMACRCR &= ~ETH_DMACRCR_RBSZ_Msk;
    ETH->DMACRCR |= ((ETH_BUF_SIZE << ETH_DMACRCR_RBSZ_Pos) & ETH_DMACRCR_RBSZ_Msk);

    ETH->MTLTQOMR |= ETH_MTLTQOMR_FTQ;

    ETH->MACCR   |= ETH_MACCR_TE | ETH_MACCR_RE;

    ETH->DMACTCR |= ETH_DMACTCR_ST;
    ETH->DMACRCR |= ETH_DMACRCR_SR;

    return 1U;
}

uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr)
{
    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) != 0U){}

    uint32_t reg = ETH->MACMDIOAR;

    reg &= ~(ETH_MACMDIOAR_PA_Msk  | ETH_MACMDIOAR_RDA_Msk | ETH_MACMDIOAR_MOC_Msk | ETH_MACMDIOAR_C45E);

    reg |= ((uint32_t)(phy_addr & 0x1FU) << ETH_MACMDIOAR_PA_Pos);
    reg |= ((uint32_t)(reg_addr & 0x1FU) << ETH_MACMDIOAR_RDA_Pos);

    reg |= ETH_MACMDIOAR_MOC_RD;

    ETH->MACMDIOAR = reg | ETH_MACMDIOAR_MB;

    while ((ETH->MACMDIOAR & ETH_MACMDIOAR_MB) != 0U){}

    return (uint16_t)(ETH->MACMDIODR & ETH_MACMDIODR_MD_Msk);
}

// uint8_t ethernet_send_test_frame(void)
// {
//     if ((tx_desc.DESC3 & TDES3_OWN) != 0U)
//     {
//         return 0U;
//     }

//     uint16_t len = eth_build_test_frame();

//     tx_desc.DESC0 = (uint32_t)tx_buffer;
//     tx_desc.DESC1 = 0U;
//     tx_desc.DESC2 = len & TDES2_B1L_Msk;

//     tx_desc.DESC3 = TDES3_FD | TDES3_LD | (len & TDES3_FL_Msk);

//     __DSB();

//     tx_desc.DESC3 |= TDES3_OWN;

//     __DSB();

//     ETH->DMACTDTPR = (uint32_t)&tx_desc;

//     return 1U;
// }

uint8_t ethernet_send_test_frame(void)
{
    uint16_t len = eth_build_test_frame();

    return ethernet_tx_start(len);
}

uint8_t ethernet_rx_poll(uint8_t **frame, uint16_t *length)
{
    eth_dma_desc_t *desc = &rx_desc[rx_index];

    if ((desc->DESC3 & RDES3_OWN) != 0U)
    {
        return 0U;
    }

    uint32_t status = desc->DESC3;

    if (((status & RDES3_FD) != 0U) &&
        ((status & RDES3_LD) != 0U) &&
        ((status & RDES3_ES) == 0U))
    {
        *length = (uint16_t)(status & RDES3_PL_Msk);
        *frame  = rx_buffer[rx_index];

        return 1U;
    }

    ethernet_rx_release();
    return 0U;
}

void ethernet_rx_release(void)
{
    eth_dma_desc_t *desc = &rx_desc[rx_index];

    desc->DESC0 = (uint32_t)rx_buffer[rx_index];
    desc->DESC1 = 0U;
    desc->DESC2 = 0U;
    desc->DESC3 = RDES3_BUF1V;

    __DSB();

    desc->DESC3 |= RDES3_OWN;

    __DSB();

    ETH->DMACRDTPR = (uint32_t)desc;

    rx_index++;

    if (rx_index >= ETH_RX_DESC_COUNT)
    {
        rx_index = 0U;
    }
}

static uint16_t eth_read_u16_be(const uint8_t *data)
{
    return ((uint16_t)data[0] << 8U) |
            (uint16_t)data[1];
}

static uint8_t ethernet_handle_arp(const uint8_t *frame, uint16_t length)
{
    if (length < 42U)
    {
        return 0U;
    }

    uint16_t htype  = eth_read_u16_be(&frame[14]);
    uint16_t ptype  = eth_read_u16_be(&frame[16]);
    uint16_t opcode = eth_read_u16_be(&frame[20]);

    if (htype != ARP_HTYPE_ETH)
    {
        return 0U;
    }

    if (ptype != ARP_PTYPE_IPV4)
    {
        return 0U;
    }

    if (frame[18] != 6U || frame[19] != 4U)
    {
        return 0U;
    }

    if (opcode != ARP_REQUEST)
    {
        return 0U;
    }

    /* Czy pytanie jest o NASZ adres IP? */
    if ((frame[38] != eth_ip[0]) ||
        (frame[39] != eth_ip[1]) ||
        (frame[40] != eth_ip[2]) ||
        (frame[41] != eth_ip[3]))
    {
        return 0U;
    }

    /*
     * Zachowujemy dane pytającego, zanim zaczniemy nadpisywać tx_buffer.
     */
    const uint8_t *sender_mac = &frame[22];
    const uint8_t *sender_ip  = &frame[28];

    /* Ethernet Destination = MAC pytającego */
    for (uint32_t i = 0U; i < 6U; i++)
    {
        tx_buffer[i] = sender_mac[i];
    }

    /* Ethernet Source = nasz MAC */
    for (uint32_t i = 0U; i < 6U; i++)
    {
        tx_buffer[6U + i] = eth_mac[i];
    }

    /* EtherType = ARP */
    tx_buffer[12] = 0x08U;
    tx_buffer[13] = 0x06U;


    /* ARP Hardware Type = Ethernet */
    tx_buffer[14] = 0x00U;
    tx_buffer[15] = 0x01U;

    /* Protocol Type = IPv4 */
    tx_buffer[16] = 0x08U;
    tx_buffer[17] = 0x00U;

    /* MAC length / IPv4 length */
    tx_buffer[18] = 6U;
    tx_buffer[19] = 4U;

    /* Opcode = Reply */
    tx_buffer[20] = 0x00U;
    tx_buffer[21] = 0x02U;


    /* Sender MAC = STM32 */
    for (uint32_t i = 0U; i < 6U; i++)
    {
        tx_buffer[22U + i] = eth_mac[i];
    }

    /* Sender IP = STM32 */
    for (uint32_t i = 0U; i < 4U; i++)
    {
        tx_buffer[28U + i] = eth_ip[i];
    }

    /* Target MAC = urządzenie pytające */
    for (uint32_t i = 0U; i < 6U; i++)
    {
        tx_buffer[32U + i] = sender_mac[i];
    }

    /* Target IP = urządzenie pytające */
    for (uint32_t i = 0U; i < 4U; i++)
    {
        tx_buffer[38U + i] = sender_ip[i];
    }

    /*
     * Ethernet minimum:
     * 42 B Ethernet+ARP
     * +18 B padding
     * = 60 B przed FCS.
     */
    for (uint32_t i = 42U; i < ETH_MIN_FRAME_SIZE; i++)
    {
        tx_buffer[i] = 0U;
    }

    return ethernet_tx_start(ETH_MIN_FRAME_SIZE);
}

void ethernet_process_frame(uint8_t *frame, uint16_t length)
{
    if (length < 14U)
    {
        return;
    }

    uint16_t ethertype =
        ((uint16_t)frame[12] << 8U) |
         (uint16_t)frame[13];

    if (ethertype == ETH_TYPE_ARP)
    {
        ethernet_handle_arp(frame, length);
    }
    else if (ethertype == ETH_TYPE_IPV4)
    {
        ethernet_handle_ipv4(frame, length);
    }
}