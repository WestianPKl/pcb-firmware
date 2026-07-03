#include "can.h"
#include "main.h"
#include "support.h"

#define CAN_RX 0U // PD0
#define CAN_TX 1U // PD1
#define CAN_TIMEOUT  1000000U

volatile uint32_t can_diag = 0;

static uint8_t can_wait_set(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t t = CAN_TIMEOUT;
    while (((*reg & mask) == 0U) && (t > 0U)) {
        t--;
    }
    return ((*reg & mask) != 0U) ? 1U : 0U;
}

static uint8_t can_wait_clear(volatile uint32_t *reg, uint32_t mask)
{
    uint32_t t = CAN_TIMEOUT;
    while (((*reg & mask) != 0U) && (t > 0U)) {
        t--;
    }
    return ((*reg & mask) == 0U) ? 1U : 0U;
}

static void can_filter_set(uint32_t fr1, uint32_t fr2)
{
    CAN1->FMR |= CAN_FMR_FINIT;

    CAN1->FMR &= ~CAN_FMR_CAN2SB_Msk;
    CAN1->FMR |=  (14U << CAN_FMR_CAN2SB_Pos);

    CAN1->FA1R  &= ~CAN_FA1R_FACT0;
    CAN1->FS1R  |=  CAN_FS1R_FSC0;
    CAN1->FM1R  &= ~CAN_FM1R_FBM0;
    CAN1->FFA1R &= ~CAN_FFA1R_FFA0;

    CAN1->sFilterRegister[0].FR1 = fr1;
    CAN1->sFilterRegister[0].FR2 = fr2;

    CAN1->FA1R |= CAN_FA1R_FACT0;
    CAN1->FMR &= ~CAN_FMR_FINIT;
}

void can_filter_accept_all(void)
{
    can_filter_set(0x00000000U, 0x00000000U);
}

void can_filter_config(uint16_t std_id)
{
    can_filter_set((uint32_t)(std_id & 0x7FFU) << 21U, (uint32_t)0x7FFU << 21U);
}

void can_params_init(uint8_t mode)
{
    can_diag = 0U;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIODEN;
    (void)RCC->AHB1ENR;

    GPIOD->MODER &= ~((3U << (CAN_RX * 2U)) | (3U << (CAN_TX * 2U)));
    GPIOD->MODER |=  ((2U << (CAN_RX * 2U)) | (2U << (CAN_TX * 2U)));

    GPIOD->PUPDR &= ~(3U << (CAN_RX * 2U));
    GPIOD->PUPDR |=  (1U << (CAN_RX * 2U));

    GPIOD->AFR[0] &= ~((0xFU << (CAN_RX * 4U)) | (0xFU << (CAN_TX * 4U)));
    GPIOD->AFR[0] |=  ((0x9U << (CAN_RX * 4U)) | (0x9U << (CAN_TX * 4U)));

    NVIC_SetPriority(CAN1_RX0_IRQn, 5);
    NVIC_EnableIRQ(CAN1_RX0_IRQn);

    RCC->APB1ENR |= RCC_APB1ENR_CAN1EN;
    (void)RCC->APB1ENR;

    CAN1->MCR &= ~CAN_MCR_SLEEP;
    if (!can_wait_clear(&CAN1->MSR, CAN_MSR_SLAK)) {
        can_diag = 1U;
        return;
    }

    CAN1->MCR |= CAN_MCR_INRQ;
    if (!can_wait_set(&CAN1->MSR, CAN_MSR_INAK)) {
        can_diag = 2U;
        return;
    }

    CAN1->MCR |= CAN_MCR_ABOM;
    CAN1->MCR |= CAN_MCR_NART;
    CAN1->MCR &= ~CAN_MCR_TTCM;
    CAN1->MCR &= ~CAN_MCR_RFLM;
    CAN1->MCR &= ~CAN_MCR_TXFP;

    uint32_t pclk1    = apb1_pclk_hz();
    uint32_t baud     = 500000U;
    uint32_t bs1      = 15U, bs2 = 2U;
    uint32_t total_tq = 1U + bs1 + bs2;
    uint32_t brp      = pclk1 / (baud * total_tq);

    CAN1->BTR = (0U   << CAN_BTR_SJW_Pos)
            | ((bs1 - 1U)  << CAN_BTR_TS1_Pos)
            | ((bs2 - 1U)  << CAN_BTR_TS2_Pos)
            | ((brp - 1U)  << CAN_BTR_BRP_Pos);

    if (mode == CAN_MODE_LOOPBACK) {
        CAN1->BTR |= (CAN_BTR_LBKM | CAN_BTR_SILM);
    } else {
        CAN1->BTR &= ~(CAN_BTR_LBKM | CAN_BTR_SILM);
    }

    can_diag = 10U;
}

void can_start(void)
{
    CAN1->MCR &= ~CAN_MCR_INRQ;

    if (!can_wait_clear(&CAN1->MSR, CAN_MSR_INAK)) {
        can_diag = 3U;
        return;
    }

    while ((CAN1->RF0R & CAN_RF0R_FMP0) != 0U) {
        CAN1->RF0R = CAN_RF0R_RFOM0;
    }

    CAN1->IER |= CAN_IER_FMPIE0;
    can_diag = 11U;
}

uint8_t can_add_tx_message(can_tx_header_typedef *pHeader, uint8_t aData[], uint32_t *pTxMailbox)
{
    uint32_t transmitmailbox;
    uint32_t tsr;
    uint32_t tdlr = 0U;
    uint32_t tdhr = 0U;
    uint8_t dlc;
    uint8_t i;

    if ((pHeader == 0) || (aData == 0) || (pTxMailbox == 0)) {
        can_diag = 20U;
        return 1U;
    }

    tsr = CAN1->TSR;

    if ((tsr & (CAN_TSR_TME0 | CAN_TSR_TME1 | CAN_TSR_TME2)) == 0U) {
        can_diag = 21U;
        return 1U;
    }

    transmitmailbox = (tsr & CAN_TSR_CODE) >> CAN_TSR_CODE_Pos;
    if (transmitmailbox > 2U) {
        can_diag = 22U;
        return 1U;
    }

    *pTxMailbox = (uint32_t)1U << transmitmailbox;

    if (pHeader->ide == CAN_ID_STD) {
        CAN1->sTxMailBox[transmitmailbox].TIR =
            ((pHeader->std_id & 0x7FFU) << CAN_TI0R_STID_Pos) |
            (pHeader->rtr ? CAN_TI0R_RTR : 0U);
    } else {
        CAN1->sTxMailBox[transmitmailbox].TIR =
            ((pHeader->ext_id & 0x1FFFFFFFU) << CAN_TI0R_EXID_Pos) |
            CAN_TI0R_IDE |
            (pHeader->rtr ? CAN_TI0R_RTR : 0U);
    }

    dlc = (uint8_t)(pHeader->dlc & 0x0FU);
    if (dlc > 8U) {
        dlc = 8U;
    }

    CAN1->sTxMailBox[transmitmailbox].TDTR = dlc;

    if (pHeader->transmit_global_time == 1U) {
        CAN1->sTxMailBox[transmitmailbox].TDTR |= CAN_TDT0R_TGT;
    }

    for (i = 0; i < dlc; i++) {
        if (i < 4U) {
            tdlr |= ((uint32_t)aData[i] << (8U * i));
        } else {
            tdhr |= ((uint32_t)aData[i] << (8U * (i - 4U)));
        }
    }

    CAN1->sTxMailBox[transmitmailbox].TDLR = tdlr;
    CAN1->sTxMailBox[transmitmailbox].TDHR = tdhr;
    CAN1->sTxMailBox[transmitmailbox].TIR |= CAN_TI0R_TXRQ;

    can_diag = 30U;
    return 0U;
}

uint8_t can_get_rx_message(uint32_t RxFifo, can_rx_header_typedef *pHeader, uint8_t aData[])
{
    uint32_t rir, rdtr, rdlr, rdhr;

    if ((pHeader == 0) || (aData == 0)) {
        can_diag = 40U;
        return 1U;
    }

    if (RxFifo == CAN_RX_FIFO0) {
        if ((CAN1->RF0R & CAN_RF0R_FMP0) == 0U) {
            can_diag = 41U;
            return 1U;
        }
    } else {
        if ((CAN1->RF1R & CAN_RF1R_FMP1) == 0U) {
            can_diag = 42U;
            return 1U;
        }
    }

    rir  = CAN1->sFIFOMailBox[RxFifo].RIR;
    rdtr = CAN1->sFIFOMailBox[RxFifo].RDTR;
    rdlr = CAN1->sFIFOMailBox[RxFifo].RDLR;
    rdhr = CAN1->sFIFOMailBox[RxFifo].RDHR;

    pHeader->ide = (rir & CAN_RI0R_IDE) ? CAN_ID_EXT : CAN_ID_STD;

    if (pHeader->ide == CAN_ID_STD) {
        pHeader->std_id = (rir & CAN_RI0R_STID) >> CAN_RI0R_STID_Pos;
        pHeader->ext_id = 0U;
    } else {
        pHeader->ext_id = ((rir & (CAN_RI0R_EXID | CAN_RI0R_STID)) >> CAN_RI0R_EXID_Pos);
        pHeader->std_id = 0U;
    }

    pHeader->rtr = (rir & CAN_RI0R_RTR) ? 1U : 0U;
    pHeader->dlc = (rdtr & CAN_RDT0R_DLC) >> CAN_RDT0R_DLC_Pos;
    pHeader->filter_match_index = (rdtr & CAN_RDT0R_FMI) >> CAN_RDT0R_FMI_Pos;
    pHeader->timestamp = (rdtr & CAN_RDT0R_TIME) >> CAN_RDT0R_TIME_Pos;

    aData[0] = (uint8_t)((rdlr & CAN_RDL0R_DATA0) >> CAN_RDL0R_DATA0_Pos);
    aData[1] = (uint8_t)((rdlr & CAN_RDL0R_DATA1) >> CAN_RDL0R_DATA1_Pos);
    aData[2] = (uint8_t)((rdlr & CAN_RDL0R_DATA2) >> CAN_RDL0R_DATA2_Pos);
    aData[3] = (uint8_t)((rdlr & CAN_RDL0R_DATA3) >> CAN_RDL0R_DATA3_Pos);
    aData[4] = (uint8_t)((rdhr & CAN_RDH0R_DATA4) >> CAN_RDH0R_DATA4_Pos);
    aData[5] = (uint8_t)((rdhr & CAN_RDH0R_DATA5) >> CAN_RDH0R_DATA5_Pos);
    aData[6] = (uint8_t)((rdhr & CAN_RDH0R_DATA6) >> CAN_RDH0R_DATA6_Pos);
    aData[7] = (uint8_t)((rdhr & CAN_RDH0R_DATA7) >> CAN_RDH0R_DATA7_Pos);

    if (RxFifo == CAN_RX_FIFO0) {
        CAN1->RF0R = CAN_RF0R_RFOM0;
    } else {
        CAN1->RF1R = CAN_RF1R_RFOM1;
    }

    can_diag = 50U;
    return 0U;
}

volatile uint32_t can_isr_count = 0;
can_rx_header_typedef     can_rx_header;
uint8_t                   can_rx_data[8];


void CAN1_RX0_IRQHandler(void)
{
    can_isr_count++;
    if ((CAN1->RF0R & CAN_RF0R_FMP0) != 0U) {
        if (can_get_rx_message(CAN_RX_FIFO0, &can_rx_header, can_rx_data) == 0U) {
            BaseType_t xHigherPriorityTaskWoken = pdFALSE;
            vTaskNotifyGiveFromISR(task_can_handler, &xHigherPriorityTaskWoken);
            portYIELD_FROM_ISR(xHigherPriorityTaskWoken);
        } else {
            CAN1->RF0R = CAN_RF0R_RFOM0;
        }
    }
}