#ifndef FAULT_H
#define FAULT_H

#include <stdint.h>
#include "stm32c5xx.h"

#define FAULT_PIN 14U // PB14

#define FAULT_NONE               0x0000U
#define FAULT_BOARD_INFO         0x0001U
#define FAULT_CAN_BUS_OFF        0x0002U
#define FAULT_CAN_ERROR_PASSIVE  0x0004U
#define FAULT_CAN_ERROR_WARNING  0x0008U
#define FAULT_DIGITAL_OUTPUT     0x0010U
#define FAULT_INTERNAL           0x0020U
#define FAULT_PWM                0x0040U

void fault_pin_init(void);
uint16_t fault_get(void);
uint8_t fault_active(void);
void fault_set(uint16_t fault);
void fault_clear(uint16_t fault);

#endif // FAULT_H