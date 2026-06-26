#include "ethernet.h"
#include "systick.h"

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
#define TDES0_TCH  (1U << 20)
#define RDES0_OWN  (1U << 31)
#define RDES0_ES   (1U << 15)
#define RDES1_RCH  (1U << 14)
#define RDES1_RBS1 (0x1FFFU)

#define ETH_RX_DESC_CNT 4U
#define ETH_TX_DESC_CNT 4U
#define ETH_BUF_SIZE    1524U

typedef struct {
    volatile uint32_t des0;
    volatile uint32_t des1;
    volatile uint32_t des2;
    volatile uint32_t des3;
} eth_desc_t;

static eth_desc_t rx_desc[ETH_RX_DESC_CNT] __attribute__((aligned(4)));
static eth_desc_t tx_desc[ETH_TX_DESC_CNT] __attribute__((aligned(4)));
static uint8_t    rx_buf[ETH_RX_DESC_CNT][ETH_BUF_SIZE] __attribute__((aligned(4)));
static uint8_t    tx_buf[ETH_TX_DESC_CNT][ETH_BUF_SIZE] __attribute__((aligned(4)));

static eth_desc_t *cur_rx;
static eth_desc_t *cur_tx;

volatile uint8_t eth_rx_ready;

static void eth_pin_af11(GPIO_TypeDef *port, uint32_t pin)
{
    port->MODER   &= ~(3U << (pin * 2));
    port->MODER   |=  (2U << (pin * 2));
    port->OTYPER  &= ~(1U << pin);
    port->OSPEEDR |=  (3U << (pin * 2));
    port->PUPDR   &= ~(3U << (pin * 2));
    if (pin < 8U) {
        port->AFR[0] &= ~(0xFU << (pin * 4));
        port->AFR[0] |=  (11U  << (pin * 4));
    } else {
        port->AFR[1] &= ~(0xFU << ((pin - 8U) * 4));
        port->AFR[1] |=  (11U  << ((pin - 8U) * 4));
    }
}

static uint16_t eth_phy_read(uint16_t reg)
{
    ETH->MACMIIAR = (PHY_ADDR << 11) | ((uint32_t)reg << 6) | MACMIIAR_CR | ETH_MACMIIAR_MB;
    uint32_t t = 100000U;
    while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) && --t) {}
    return (uint16_t)ETH->MACMIIDR;
}

static void eth_phy_write(uint16_t reg, uint16_t val)
{
    ETH->MACMIIDR = val;
    ETH->MACMIIAR = (PHY_ADDR << 11) | ((uint32_t)reg << 6) | MACMIIAR_CR
                  | ETH_MACMIIAR_MW | ETH_MACMIIAR_MB;
    uint32_t t = 100000U;
    while ((ETH->MACMIIAR & ETH_MACMIIAR_MB) && --t) {}
}

static void eth_dma_desc_init(void)
{
    for (uint32_t i = 0; i < ETH_RX_DESC_CNT; i++) {
        rx_desc[i].des0 = RDES0_OWN;
        rx_desc[i].des1 = RDES1_RCH | (ETH_BUF_SIZE & RDES1_RBS1);
        rx_desc[i].des2 = (uint32_t)&rx_buf[i][0];
        rx_desc[i].des3 = (uint32_t)&rx_desc[(i + 1U) % ETH_RX_DESC_CNT];
    }
    for (uint32_t i = 0; i < ETH_TX_DESC_CNT; i++) {
        tx_desc[i].des0 = TDES0_TCH;
        tx_desc[i].des1 = 0;
        tx_desc[i].des2 = (uint32_t)&tx_buf[i][0];
        tx_desc[i].des3 = (uint32_t)&tx_desc[(i + 1U) % ETH_TX_DESC_CNT];
    }
    ETH->DMARDLAR = (uint32_t)&rx_desc[0];
    ETH->DMATDLAR = (uint32_t)&tx_desc[0];
    cur_rx = &rx_desc[0];
    cur_tx = &tx_desc[0];
}

void ethernet_init(void)
{
    RCC->APB2ENR |= RCC_APB2ENR_SYSCFGEN;
    (void)RCC->APB2ENR;
    SYSCFG->PMC |= SYSCFG_PMC_MII_RMII_SEL;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN | RCC_AHB1ENR_GPIOBEN
                  | RCC_AHB1ENR_GPIOCEN | RCC_AHB1ENR_GPIOGEN;
    (void)RCC->AHB1ENR;
    eth_pin_af11(GPIOA, 1);
    eth_pin_af11(GPIOA, 2);
    eth_pin_af11(GPIOA, 7);
    eth_pin_af11(GPIOC, 1);
    eth_pin_af11(GPIOC, 4);
    eth_pin_af11(GPIOC, 5);
    eth_pin_af11(GPIOG, 11);
    eth_pin_af11(GPIOG, 13);
    eth_pin_af11(GPIOB, 13);

    RCC->AHB1ENR |= RCC_AHB1ENR_ETHMACEN | RCC_AHB1ENR_ETHMACTXEN | RCC_AHB1ENR_ETHMACRXEN;
    (void)RCC->AHB1ENR;

    ETH->DMABMR |= ETH_DMABMR_SR;
    uint32_t t = 1000000U;
    while ((ETH->DMABMR & ETH_DMABMR_SR) && --t) {}

    eth_phy_write(PHY_BCR, PHY_BCR_RESET);
    systick_delay_ms(2);
    t = 100000U;
    while ((eth_phy_read(PHY_BCR) & PHY_BCR_RESET) && --t) {}
    eth_phy_write(PHY_BCR, PHY_BCR_AN_ENABLE | PHY_BCR_AN_RESTART);
    t = 5000000U;
    while (!(eth_phy_read(PHY_BSR) & PHY_BSR_LINK_UP) && --t) {}

    ETH->MACCR  = ETH_MACCR_FES | ETH_MACCR_DM | ETH_MACCR_IPCO;
    ETH->MACFFR = ETH_MACFFR_PM;
    ETH->MACHTHR = 0;
    ETH->MACHTLR = 0;
    ETH->MACFCR = 0;

    ETH->MACA0HR = 0x00000000U;
    ETH->MACA0LR = 0x00000002U;

    eth_dma_desc_init();

    ETH->DMAOMR = ETH_DMAOMR_TSF | ETH_DMAOMR_RSF;
    ETH->DMABMR = ETH_DMABMR_AAB | ETH_DMABMR_FB | ETH_DMABMR_USP
                | (32U << 17)
                | (32U << 8);

    ETH->MACCR  |= ETH_MACCR_TE | ETH_MACCR_RE;
    ETH->DMAOMR |= ETH_DMAOMR_FTF;
    ETH->DMAOMR |= ETH_DMAOMR_ST | ETH_DMAOMR_SR;

    ETH->DMAIER |= ETH_DMAIER_NISE | ETH_DMAIER_RIE;
    NVIC_SetPriority(ETH_IRQn, 5);
    NVIC_EnableIRQ(ETH_IRQn);
}

uint8_t eth_send_frame(const uint8_t *data, uint16_t len)
{
    if (!data) return 1;
    if (cur_tx->des0 & TDES0_OWN) return 1;
    if (len > ETH_BUF_SIZE) len = ETH_BUF_SIZE;

    uint8_t *buf = (uint8_t *)cur_tx->des2;
    for (uint16_t i = 0; i < len; i++) buf[i] = data[i];

    cur_tx->des1 = (uint32_t)len & 0x1FFFU;
    cur_tx->des0 = TDES0_TCH | TDES0_FS | TDES0_LS | TDES0_OWN;

    if (ETH->DMASR & ETH_DMASR_TBUS) {
        ETH->DMASR = ETH_DMASR_TBUS;
        ETH->DMATPDR = 0;
    }

    cur_tx = (eth_desc_t *)cur_tx->des3;
    return 0;
}

uint16_t eth_recv_frame(uint8_t *buf, uint16_t maxlen)
{
    if (!buf) return 0;
    if (cur_rx->des0 & RDES0_OWN) return 0;

    uint16_t len = 0;
    if (!(cur_rx->des0 & RDES0_ES)) {
        len = (uint16_t)((cur_rx->des0 >> 16) & 0x3FFFU);
        if (len > 4U) len -= 4U;
        if (len > maxlen) len = maxlen;
        const uint8_t *src = (const uint8_t *)cur_rx->des2;
        for (uint16_t i = 0; i < len; i++) buf[i] = src[i];
    }

    cur_rx->des0 = RDES0_OWN;

    if (ETH->DMASR & ETH_DMASR_RBUS) {
        ETH->DMASR = ETH_DMASR_RBUS;
        ETH->DMARPDR = 0;
    }

    cur_rx = (eth_desc_t *)cur_rx->des3;
    return len;
}

uint8_t eth_link_up(void)
{
    return (eth_phy_read(PHY_BSR) & PHY_BSR_LINK_UP) ? 1U : 0U;
}

void ETH_IRQHandler(void)
{
    if (ETH->DMASR & ETH_DMASR_RS) {
        ETH->DMASR = ETH_DMASR_RS;
        eth_rx_ready = 1U;
    }
    ETH->DMASR = ETH_DMASR_NIS;
}
