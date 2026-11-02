#include "can.h"
#include "stm32h563xx.h"
#include "timer.h"

#define FDCAN_RAM_SIZE        0x350U

#define FDCAN_STD_FILTER_OFS  0x000U
#define FDCAN_EXT_FILTER_OFS  0x070U
#define FDCAN_RX_FIFO0_OFS    0x0B0U
#define FDCAN_RX_FIFO1_OFS    0x188U
#define FDCAN_TX_EVENT_OFS    0x260U
#define FDCAN_TX_FIFO_OFS     0x278U

#define FDCAN_ELEMENT_SIZE 72U
#define FDCAN_TX_ELEMENT_WORDS     18U

volatile uint8_t can_rx0_irq = 0U;
volatile uint8_t can_rx0_full_irq = 0U;
volatile uint8_t can_rx0_lost_irq = 0U;

volatile uint8_t can_rx1_irq = 0U;
volatile uint8_t can_rx1_full_irq = 0U;
volatile uint8_t can_rx1_lost_irq = 0U;

volatile uint8_t can_tx_irq = 0U;
volatile uint8_t can_ew_irq = 0U;
volatile uint8_t can_ep_irq = 0U;
volatile uint8_t can_bo_irq = 0U;

static volatile uint8_t can_tx_busy = 0U;

static volatile can_state_t error_state = CAN_STATE_ACTIVE;

uint8_t can_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIODEN;
    (void)RCC->AHB2ENR;

    GPIOD->MODER &= ~(0x3U << (FDCAN1_RX_PIN * 2U) | 0x3U << (FDCAN1_TX_PIN * 2U));
    GPIOD->MODER |= (0x2U << (FDCAN1_RX_PIN * 2U) | 0x2U << (FDCAN1_TX_PIN * 2U));

    GPIOD->OTYPER &= ~((0x1U << FDCAN1_RX_PIN) | (0x1U << FDCAN1_TX_PIN));
    GPIOD->OSPEEDR |= (0x3U << (FDCAN1_RX_PIN * 2U) | 0x3U << (FDCAN1_TX_PIN * 2U)); 
    GPIOD->PUPDR &= ~(0x3U << (FDCAN1_RX_PIN * 2U) | 0x3U << (FDCAN1_TX_PIN * 2U));

    GPIOD->AFR[0] &= ~((0xFU << (FDCAN1_RX_PIN * 4U)) | (0xFU << (FDCAN1_TX_PIN * 4U)));
    GPIOD->AFR[0] |= (0x9U << (FDCAN1_RX_PIN * 4U)) | (0x9U << (FDCAN1_TX_PIN * 4U));

    RCC->CCIPR5 &= ~RCC_CCIPR5_FDCANSEL_Msk;
    RCC->CCIPR5 |= (0x1U << RCC_CCIPR5_FDCANSEL_Pos);

    RCC->APB1HENR |= RCC_APB1HENR_FDCANEN;
    (void)RCC->APB1HENR;

    FDCAN1->CCCR |= FDCAN_CCCR_INIT;
    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT) == 0U) {}

    FDCAN1->CCCR |= FDCAN_CCCR_CCE;
    while ((FDCAN1->CCCR & FDCAN_CCCR_CCE) == 0U) {}

    FDCAN1->CCCR &= ~(FDCAN_CCCR_MON | FDCAN_CCCR_DAR | FDCAN_CCCR_TEST | FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | FDCAN_CCCR_PXHD);
    FDCAN_CONFIG->CKDIV = 0U;
    FDCAN1->NBTP = (9U  << FDCAN_NBTP_NBRP_Pos) | (3U  << FDCAN_NBTP_NSJW_Pos) | (18U << FDCAN_NBTP_NTSEG1_Pos) | (4U  << FDCAN_NBTP_NTSEG2_Pos);

    FDCAN1->RXGFC = 0U;
    FDCAN1->RXGFC &= ~(FDCAN_RXGFC_ANFS_Msk | FDCAN_RXGFC_ANFE_Msk);
    FDCAN1->RXGFC |= (0x2U << FDCAN_RXGFC_ANFS_Pos) | (0x2U << FDCAN_RXGFC_ANFE_Pos);
    FDCAN1->RXGFC |= (0x2U << FDCAN_RXGFC_LSS_Pos) | (0x1U << FDCAN_RXGFC_F0OM_Pos); //ilość elementów LSS

    FDCAN1->TXBC &= ~FDCAN_TXBC_TFQM_Msk;

    volatile uint32_t *ram = (volatile uint32_t *)SRAMCAN_BASE;
    volatile uint32_t *std_filter = (volatile uint32_t *)(ram + FDCAN_STD_FILTER_OFS);

    for (uint32_t i = 0U; i < (FDCAN_RAM_SIZE / 4U); i++)
    {
        ram[i] = 0U;
    }

    // std_filter[0] = (0x1U   << 30U) | (0x1U   << 27U) | (0x100U << 16U) | (0x200U << 0U);
    // std_filter[0] = (0x1U   << 27U) | (0x100U << 16U) | (0x10FU << 0U);
    // std_filter[0] = (0x2U   << 30U) | (0x1U   << 27U) | (0x100U << 16U) | (0x7F0U << 0U);

    std_filter[0] = (0x1U   << 30U) | (0x1U   << 27U) | (0x100U << 16U) | (0x200U << 0U);
    std_filter[1] = (0x0U   << 30U) | (0x2U   << 27U) | (0x300U << 16U) | (0x30FU << 0U);

    FDCAN1->IR = FDCAN_IR_RF0N_Msk | FDCAN_IR_RF1N_Msk | FDCAN_IR_RF0F_Msk | FDCAN_IR_RF1F_Msk | FDCAN_IR_RF0L_Msk | FDCAN_IR_RF1L_Msk;
    FDCAN1->IE |= (0x1U << FDCAN_IE_RF0NE_Pos) | (0x1U << FDCAN_IE_RF1NE_Pos) | (0x1U << FDCAN_IE_RF0FE_Pos) | (0x1U << FDCAN_IE_RF1FE_Pos) | (0x1U << FDCAN_IE_RF0LE_Pos) | (0x1U << FDCAN_IE_RF1LE_Pos);
    FDCAN1->ILS &= ~((FDCAN_ILS_RXFIFO0_Msk) | (FDCAN_ILS_RXFIFO1_Msk));
    FDCAN1->ILE |= (0x1U << FDCAN_ILE_EINT0_Pos);

    FDCAN1->IR = FDCAN_IR_TC_Msk;
    FDCAN1->IE |= (0x1U << FDCAN_IE_TCE_Pos);
    FDCAN1->ILS |= (0x1U << FDCAN_ILS_SMSG_Pos);

    FDCAN1->IR = FDCAN_IR_EW_Msk | FDCAN_IR_EP_Msk | FDCAN_IR_BO_Msk;
    FDCAN1->IE |= (0x1U << FDCAN_IE_EWE_Pos) | (0x1U << FDCAN_IE_EPE_Pos) | (0x1U << FDCAN_IE_BOE_Pos);
    FDCAN1->ILS |= (0x1U << FDCAN_ILS_PERR_Pos) | (0x1U << FDCAN_ILS_BERR_Pos);

    FDCAN1->ILE |= (0x1U << FDCAN_ILE_EINT1_Pos);

    FDCAN1->TXBTIE |= (1U << 0U) | (1U << 1U) | (1U << 2U);

    NVIC->IPR[39] = 0x50U;
    NVIC->ICPR[1] = (1U << 7U);
    NVIC->ISER[1] = (1U << 7U);

    NVIC->IPR[40] = 0x50U;
    NVIC->ICPR[1] = (1U << 8U);
    NVIC->ISER[1] = (1U << 8U);

    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT_Msk;
    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT) != 0U) {}
    return 1U;
}

uint8_t can_send(uint32_t id, const uint8_t *data, uint8_t length)
{
    if ((data == NULL) || (length > 8U) || (id > 0x7FFU))
    {
        return 0U;
    }

    if (can_tx_busy != 0U)
    {
        return 0U;
    }

    if ((FDCAN1->TXFQS & FDCAN_TXFQS_TFQF_Msk) != 0U)
    {
        return 0U;
    }

    uint32_t put_index = (FDCAN1->TXFQS & FDCAN_TXFQS_TFQPI_Msk) >> FDCAN_TXFQS_TFQPI_Pos;

    volatile uint32_t *tx_element =(volatile uint32_t *)(SRAMCAN_BASE + FDCAN_TX_FIFO_OFS + (put_index * FDCAN_ELEMENT_SIZE));

    tx_element[0] = ((id & 0x7FFU) << 18U);
    tx_element[1] = ((uint32_t)length << 16U);

    for (uint8_t i = 0U; i < 16U; i++)
    {
        tx_element[2U + i] = 0U;
    }

    for (uint8_t i = 0U; i < length; i++)
    {
        uint32_t word_index = 2U + (i / 4U);
        uint32_t shift = (i % 4U) * 8U;
        tx_element[word_index] |= ((uint32_t)data[i] << shift);
    }

    can_tx_busy = 1U;
    FDCAN1->TXBAR = (1UL << put_index);

    return 1U;
}

uint8_t can_receive_fifo0(uint32_t *id, uint8_t *data, uint8_t *length)
{
    if ((id == NULL) || (data == NULL) || (length == NULL))
    {
        return 0U;
    }

    uint32_t fifo_status = FDCAN1->RXF0S;
    uint32_t fill_level = (fifo_status & FDCAN_RXF0S_F0FL_Msk) >> FDCAN_RXF0S_F0FL_Pos;

    if (fill_level == 0U)
    {
        return 0U;
    }

    uint32_t get_index = (fifo_status & FDCAN_RXF0S_F0GI_Msk) >> FDCAN_RXF0S_F0GI_Pos;
    volatile uint32_t *rx_element = (volatile uint32_t *)(SRAMCAN_BASE + FDCAN_RX_FIFO0_OFS + (get_index * FDCAN_ELEMENT_SIZE));

    if ((rx_element[0] & (1UL << 30U)) != 0U)
    {
        return 0U;
    }

    *id = (rx_element[0] >> 18U) & 0x7FFU;

    uint8_t dlc = (uint8_t)((rx_element[1] >> 16U) & 0xFU);

    if (dlc > 8U)
    {
        return 0U;
    }

    *length = dlc;

    for (uint8_t i = 0U; i < *length; i++)
    {
        uint32_t word_index = 2U + (i / 4U);
        uint32_t shift = (i % 4U) * 8U;
        data[i] = (uint8_t)((rx_element[word_index] >> shift) & 0xFFU);
    }

    FDCAN1->RXF0A = (get_index << FDCAN_RXF0A_F0AI_Pos);

    return 1U;
}

uint8_t can_receive_fifo1(uint32_t *id, uint8_t *data, uint8_t *length)
{
    if ((id == NULL) || (data == NULL) || (length == NULL))
    {
        return 0U;
    }

    uint32_t fifo_status = FDCAN1->RXF1S;
    uint32_t fill_level = (fifo_status & FDCAN_RXF1S_F1FL_Msk) >> FDCAN_RXF1S_F1FL_Pos;

    if (fill_level == 0U)
    {
        return 0U;
    }

    uint32_t get_index = (fifo_status & FDCAN_RXF1S_F1GI_Msk) >> FDCAN_RXF1S_F1GI_Pos;
    volatile uint32_t *rx_element = (volatile uint32_t *)(SRAMCAN_BASE + FDCAN_RX_FIFO1_OFS + (get_index * FDCAN_ELEMENT_SIZE));

    if ((rx_element[0] & (1UL << 30U)) != 0U)
    {
        return 0U;
    }

    *id = (rx_element[0] >> 18U) & 0x7FFU;

    uint8_t dlc = (uint8_t)((rx_element[1] >> 16U) & 0xFU);

    if (dlc > 8U)
    {
        return 0U;
    }

    *length = dlc;

    for (uint8_t i = 0U; i < *length; i++)
    {
        uint32_t word_index = 2U + (i / 4U);
        uint32_t shift = (i % 4U) * 8U;
        data[i] = (uint8_t)((rx_element[word_index] >> shift) & 0xFFU);
    }

    FDCAN1->RXF1A = (get_index << FDCAN_RXF1A_F1AI_Pos);

    return 1U;
}

can_state_t can_get_state(void)
{
    return error_state;
}

static void can_update_error_state(void)
{
    uint32_t psr = FDCAN1->PSR;

    if ((psr & FDCAN_PSR_BO_Msk) != 0U)
    {
        error_state = CAN_STATE_BUS_OFF;
    }
    else if ((psr & FDCAN_PSR_EP_Msk) != 0U)
    {
        error_state = CAN_STATE_PASSIVE;
    }
    else if ((psr & FDCAN_PSR_EW_Msk) != 0U)
    {
        error_state = CAN_STATE_WARNING;
    }
    else
    {
        error_state = CAN_STATE_ACTIVE;
    }
}

void FDCAN1_IT0_IRQHandler(void)
{
    uint32_t ir = FDCAN1->IR;
    uint32_t clear = 0U;

    if ((ir & FDCAN_IR_RF0N_Msk) != 0U)
    {
        clear |= FDCAN_IR_RF0N_Msk;
        can_rx0_irq = 1U;
    }

    if ((ir & FDCAN_IR_RF0F_Msk) != 0U)
    {
        clear |= FDCAN_IR_RF0F_Msk;
        can_rx0_full_irq = 1U;
    }

    if ((ir & FDCAN_IR_RF0L_Msk) != 0U)
    {
        clear |= FDCAN_IR_RF0L_Msk;
        can_rx0_lost_irq = 1U;
    }

    if ((ir & FDCAN_IR_RF1N_Msk) != 0U)
    {
        clear |= FDCAN_IR_RF1N_Msk;
        can_rx1_irq = 1U;
    }

    if ((ir & FDCAN_IR_RF1F_Msk) != 0U)
    {
        clear |= FDCAN_IR_RF1F_Msk;
        can_rx1_full_irq = 1U;
    }

    if ((ir & FDCAN_IR_RF1L_Msk) != 0U)
    {
        clear |= FDCAN_IR_RF1L_Msk;
        can_rx1_lost_irq = 1U;
    }

    if (clear != 0U)
    {
        FDCAN1->IR = clear;
    }
}

void FDCAN1_IT1_IRQHandler(void)
{
    uint32_t ir = FDCAN1->IR;
    uint32_t clear = 0U;

    if ((ir & FDCAN_IR_TC_Msk) != 0U)
    {
        clear |= FDCAN_IR_TC_Msk;
        can_tx_busy = 0U;
        can_tx_irq = 1U;
    }

    if ((ir & FDCAN_IR_EW_Msk) != 0U)
    {
        clear |= FDCAN_IR_EW_Msk;
        can_ew_irq = 1U;
    }

    if ((ir & FDCAN_IR_EP_Msk) != 0U)
    {
        clear |= FDCAN_IR_EP_Msk;
        can_ep_irq = 1U;
    }

    if ((ir & FDCAN_IR_BO_Msk) != 0U)
    {
        clear |= FDCAN_IR_BO_Msk;
        can_bo_irq = 1U;
    }

    if (clear != 0U)
    {
        FDCAN1->IR = clear;
    }

    if ((ir & (FDCAN_IR_EW_Msk | FDCAN_IR_EP_Msk | FDCAN_IR_BO_Msk)) != 0U)
    {
        can_update_error_state();
    }
}