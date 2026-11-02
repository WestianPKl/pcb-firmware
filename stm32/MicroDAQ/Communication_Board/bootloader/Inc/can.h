#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "stm32c5xx.h"

#define FDCAN1_RX_PIN 11U // PA11
#define FDCAN1_TX_PIN 12U // PA12

typedef enum
{
    CAN_STATE_ACTIVE,
    CAN_STATE_WARNING,
    CAN_STATE_PASSIVE,
    CAN_STATE_BUS_OFF
} can_state_t;

typedef enum
{
    CAN_ID_STD,
    CAN_ID_EXT
} can_id_type_t;

extern volatile uint8_t can_rx0_irq;
extern volatile uint8_t can_rx0_full_irq;
extern volatile uint8_t can_rx0_lost_irq;

extern volatile uint8_t can_rx1_irq;
extern volatile uint8_t can_rx1_full_irq;
extern volatile uint8_t can_rx1_lost_irq;

extern volatile uint8_t can_tx_irq;
extern volatile uint8_t can_ew_irq;
extern volatile uint8_t can_ep_irq;
extern volatile uint8_t can_bo_irq;

can_state_t can_get_state(void);
uint16_t can_id_make(uint8_t slot, uint8_t offset);
uint8_t can_init(void);
uint8_t can_send_std(uint32_t id, const uint8_t *data, uint8_t length);
uint8_t can_receive_fifo0(uint32_t *id, can_id_type_t *id_type, uint8_t *data, uint8_t *length);

#endif // CAN_H

