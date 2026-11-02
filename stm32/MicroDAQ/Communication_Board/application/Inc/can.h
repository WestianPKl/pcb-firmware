#ifndef CAN_H
#define CAN_H

#include <stdint.h>
#include "support.h"
#include "stm32c5xx.h"

#define FDCAN1_RX_PIN 11U // PA11
#define FDCAN1_TX_PIN 12U // PA12

#define FDCAN2_RX_PIN 3U // PB3
#define FDCAN2_TX_PIN 4U // PB4

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

extern volatile uint8_t can2_rx0_irq;
extern volatile uint8_t can2_rx0_full_irq;
extern volatile uint8_t can2_rx0_lost_irq;

extern volatile uint8_t can2_rx1_irq;
extern volatile uint8_t can2_rx1_full_irq;
extern volatile uint8_t can2_rx1_lost_irq;

extern volatile uint8_t can2_tx_irq;
extern volatile uint8_t can2_ew_irq;
extern volatile uint8_t can2_ep_irq;
extern volatile uint8_t can2_bo_irq;
extern volatile uint8_t can2_tx_fail_irq;
extern volatile uint8_t can2_tx_buffer_index;

typedef struct
{
    uint32_t nominal_bitrate;
    uint32_t data_bitrate;
    uint8_t fd_enabled;
    uint8_t brs_enabled;
    uint8_t mode;
    uint8_t can_termination;
} can_config_t;

typedef struct
{
    uint32_t id;
    uint32_t period_ms;
    uint8_t id_type;
    uint8_t fd;
    uint8_t brs;
    uint8_t length;
    uint8_t data[64];
} can_cyclic_frame_t;

uint16_t can_id_make(uint8_t slot, uint8_t offset);

can_state_t can_get_state(void);
uint8_t can_init(void);
void can_recovery_start(void);
uint8_t can_send_std(uint32_t id, const uint8_t *data, uint8_t length);
uint8_t can_send_fd(uint32_t id, const uint8_t *data, uint8_t length);
uint8_t can_receive_fifo0(uint32_t *id, can_id_type_t *id_type, uint8_t *data, uint8_t *length);

can_state_t can2_get_state(void);
uint8_t can2_init(void);
uint8_t can2_config(const can_config_t *config);
void can2_recovery_start(void);
uint8_t can2_send_std(uint32_t id, const uint8_t *data, uint8_t length);
uint8_t can2_send_fd(uint32_t id, const uint8_t *data, uint8_t length);
uint8_t can2_receive_fifo0(uint32_t *id, can_id_type_t *id_type, uint8_t *data, uint8_t *length, uint8_t *fd, uint8_t *brs);

#endif // CAN_H

