#include "ethernet.h"
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

typedef struct
{
    volatile uint32_t DESC0;
    volatile uint32_t DESC1;
    volatile uint32_t DESC2;
    volatile uint32_t DESC3;
} eth_dma_desc_t;

static eth_dma_desc_t tx_desc __attribute__((aligned(32)));
static uint8_t tx_buffer[ETH_BUF_SIZE] __attribute__((aligned(32)));

static uint32_t rx_index = 0U;
static eth_dma_desc_t rx_desc[ETH_RX_DESC_COUNT]
    __attribute__((aligned(32)));
static uint8_t rx_buffer[ETH_RX_DESC_COUNT][ETH_BUF_SIZE]
    __attribute__((aligned(32)));


static uint8_t eth_mac[6];

static void ethernet_generate_mac(void)
{
    uint32_t uid0 = *(volatile uint32_t *)(UID_BASE + 0U);
    uint32_t uid1 = *(volatile uint32_t *)(UID_BASE + 4U);
    uint32_t uid2 = *(volatile uint32_t *)(UID_BASE + 8U);

    eth_mac[0] = 0x02U;

    eth_mac[1] = (uint8_t)(uid0 >> 0U);
    eth_mac[2] = (uint8_t)(uid0 >> 8U);
    eth_mac[3] = (uint8_t)(uid1 >> 0U);
    eth_mac[4] = (uint8_t)(uid1 >> 8U);
    eth_mac[5] = (uint8_t)((uid2 ^ (uid2 >> 16U)) & 0xFFU);
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


static uint16_t eth_phy_read(uint8_t phy_addr, uint8_t reg_addr)
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

uint8_t ethernet_init(void)
{

    ethernet_generate_mac();
    
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

const uint8_t *ethernet_get_mac(void)
{
    return eth_mac;
}

uint8_t ethernet_tx(const uint8_t *frame, uint16_t length)
{
    if ((frame == NULL) || (length < 14U) || (length > ETH_BUF_SIZE))
    {
        return 0U;
    }

    if ((tx_desc.DESC3 & TDES3_OWN) != 0U)
    {
        return 0U;
    }

    for (uint16_t i = 0U; i < length; i++)
    {
        tx_buffer[i] = frame[i];
    }

    return ethernet_tx_start(length);
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

