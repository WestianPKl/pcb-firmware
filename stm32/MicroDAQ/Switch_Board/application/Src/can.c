#include "can.h"
#include "input.h"
#include <stddef.h>

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

static volatile can_state_t error_state = CAN_STATE_ACTIVE;

static uint8_t can_fd_length_to_dlc(uint8_t length)
{
    if (length <= 8U)
    {
        return length;
    }

    switch (length)
    {
        case 12U: return 9U;
        case 16U: return 10U;
        case 20U: return 11U;
        case 24U: return 12U;
        case 32U: return 13U;
        case 48U: return 14U;
        case 64U: return 15U;
        default:  return 0xFFU;
    }
}

static uint8_t can_dlc_to_fd_length(uint8_t dlc)
{
    if (dlc <= 8U)
    {
        return dlc;
    }

    switch (dlc)
    {
        case 9U:  return 12U;
        case 10U: return 16U;
        case 11U: return 20U;
        case 12U: return 24U;
        case 13U: return 32U;
        case 14U: return 48U;
        case 15U: return 64U;
        default:  return 0xFFU;
    }
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

uint16_t can_id_make(uint8_t slot, uint8_t offset)
{
    return ((uint16_t)slot << 8U) | offset;
}

uint8_t can_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER &= ~(GPIO_MODER_MODE11_Msk | GPIO_MODER_MODE12_Msk);
    GPIOA->MODER |= (GPIO_MODER_MODE11_1 | GPIO_MODER_MODE12_1);

    GPIOA->OTYPER &= ~(GPIO_OTYPER_OT11_Msk | GPIO_OTYPER_OT12_Msk);
    GPIOA->OSPEEDR |= (GPIO_OSPEEDR_OSPEED11_Msk | GPIO_OSPEEDR_OSPEED12_Msk);
    GPIOA->PUPDR &= ~(GPIO_PUPDR_PUPD11_Msk | GPIO_PUPDR_PUPD12_Msk);

    GPIOA->AFR[1] &= ~(GPIO_AFRH_AFSEL11_Msk | GPIO_AFRH_AFSEL12_Msk);
    GPIOA->AFR[1] |= (0x9U << GPIO_AFRH_AFSEL11_Pos) | (0x9U << GPIO_AFRH_AFSEL12_Pos);
    
    RCC->CCIPR1 &= ~RCC_CCIPR1_FDCANSEL_Msk;

    RCC->APB1HENR |= RCC_APB1HENR_FDCANEN;
    (void)RCC->APB1HENR;

    FDCAN1->CCCR |= FDCAN_CCCR_INIT;
    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT) == 0U) {}

    FDCAN1->CCCR |= FDCAN_CCCR_CCE;
    while ((FDCAN1->CCCR & FDCAN_CCCR_CCE) == 0U) {}

    FDCAN1->CCCR &= ~(FDCAN_CCCR_MON | FDCAN_CCCR_DAR | FDCAN_CCCR_TEST | FDCAN_CCCR_FDOE | FDCAN_CCCR_BRSE | FDCAN_CCCR_PXHD);
    FDCAN1->CCCR |= FDCAN_CCCR_FDOE;
    FDCAN_CONFIG->CKDIV = 0U;

    FDCAN1->NBTP =
        (5U  << FDCAN_NBTP_NBRP_Pos)   |
        (12U << FDCAN_NBTP_NTSEG1_Pos) |
        (1U  << FDCAN_NBTP_NTSEG2_Pos) |
        (1U  << FDCAN_NBTP_NSJW_Pos);
    FDCAN1->DBTP =
        (0U  << FDCAN_DBTP_DBRP_Pos)   |
        (17U << FDCAN_DBTP_DTSEG1_Pos) |
        (4U  << FDCAN_DBTP_DTSEG2_Pos) |
        (3U  << FDCAN_DBTP_DSJW_Pos)   |
        FDCAN_DBTP_TDC_Msk;
    FDCAN1->TDCR = (18U << FDCAN_TDCR_TDCO_Pos) | (0U  << FDCAN_TDCR_TDCF_Pos);

    FDCAN1->RXGFC = 0U;
    FDCAN1->RXGFC &= ~(FDCAN_RXGFC_ANFS_Msk | FDCAN_RXGFC_ANFE_Msk);
    FDCAN1->RXGFC |= (0x2U << FDCAN_RXGFC_ANFS_Pos) | (0x2U << FDCAN_RXGFC_ANFE_Pos) | (0x1U << FDCAN_RXGFC_LSS_Pos);
    FDCAN1->RXGFC &= ~FDCAN_RXGFC_F0OM_Msk;

    FDCAN1->TXBC &= ~FDCAN_TXBC_TFQM_Msk;

    volatile uint32_t *ram = (volatile uint32_t *)SRAMCAN_BASE;
    volatile uint32_t *std_filter = (volatile uint32_t *)(SRAMCAN_BASE + FDCAN_STD_FILTER_OFS);

    for (uint32_t i = 0U; i < (FDCAN_RAM_SIZE / 4U); i++)
    {
        ram[i] = 0U;
    }

    uint16_t slot_base_id = ((uint16_t)slot_id << 8U);
    std_filter[0] = (0x2U   << 30U) | (0x1U   << 27U) | (slot_base_id << 16U) | 0x700U;

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

    NVIC_SetPriority(FDCAN1_IT0_IRQn, 4U);
    NVIC_ClearPendingIRQ(FDCAN1_IT0_IRQn);
    NVIC_EnableIRQ(FDCAN1_IT0_IRQn);

    NVIC_SetPriority(FDCAN1_IT1_IRQn, 5U);
    NVIC_ClearPendingIRQ(FDCAN1_IT1_IRQn);
    NVIC_EnableIRQ(FDCAN1_IT1_IRQn);

    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT_Msk;
    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT) != 0U) {}
    return 1U;
}

void can_recovery_start(void)
{
    FDCAN1->CCCR |= FDCAN_CCCR_INIT_Msk;

    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT_Msk) == 0U) {}

    FDCAN1->IR = FDCAN_IR_BO_Msk | FDCAN_IR_EP_Msk | FDCAN_IR_EW_Msk;
    FDCAN1->CCCR &= ~FDCAN_CCCR_INIT_Msk;

    while ((FDCAN1->CCCR & FDCAN_CCCR_INIT_Msk) != 0U){}

    can_update_error_state();
}

uint8_t can_send_std(uint32_t id, const uint8_t *data, uint8_t length)
{
    if ((data == NULL) || (length > 8U) || (id > 0x7FFU))
    {
        return 0U;
    }

    if ((FDCAN1->TXFQS & FDCAN_TXFQS_TFQF_Msk) != 0U)
    {
        return 0U;
    }

    uint32_t put_index = (FDCAN1->TXFQS & FDCAN_TXFQS_TFQPI_Msk) >> FDCAN_TXFQS_TFQPI_Pos;

    volatile uint32_t *tx_element = (volatile uint32_t *) (SRAMCAN_BASE + FDCAN_TX_FIFO_OFS + (put_index * FDCAN_ELEMENT_SIZE));

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

    FDCAN1->TXBAR = (1UL << put_index);

    return 1U;
}

uint8_t can_send_fd(uint32_t id, const uint8_t *data, uint8_t length)
{
    if ((data == NULL) || (length > 64U) || (id > 0x7FFU))
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
    
    uint8_t dlc = can_fd_length_to_dlc(length);

    if (dlc == 0xFFU)
    {
        return 0U;
    }

    tx_element[1] = ((uint32_t)dlc << 16U) | (1UL << 21U) | (1UL << 20U);

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

    FDCAN1->TXBAR = (1UL << put_index);
    return 1U;
}

uint8_t can_receive_fifo0(uint32_t *id, can_id_type_t *id_type, uint8_t *data, uint8_t *length)
{
    if ((id == NULL) || (id_type == NULL) || (data == NULL) || (length == NULL))
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
        *id_type = CAN_ID_EXT;
        *id = rx_element[0] & 0x1FFFFFFFU;
    }
    else
    {
        *id_type = CAN_ID_STD;
        *id = (rx_element[0] >> 18U) & 0x7FFU;
    }

    uint8_t dlc = (uint8_t)((rx_element[1] >> 16U) & 0xFU);

    dlc = can_dlc_to_fd_length(dlc);

    if (dlc == 0xFFU)
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

can_state_t can_get_state(void)
{
    return error_state;
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