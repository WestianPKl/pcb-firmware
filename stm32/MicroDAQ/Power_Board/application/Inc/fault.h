#ifndef FAULT_H
#define FAULT_H

#include <stdint.h>

#define FAULT_PIN 8U // PA8

#define FAULT_NONE              0x0000U
#define FAULT_BOARD_INFO        (1U << 0)
#define FAULT_INA               (1U << 1)
#define FAULT_VIN_LINE          (1U << 2)
#define FAULT_12V_LINE          (1U << 3)
#define FAULT_5V_LINE           (1U << 4)
#define FAULT_INTERNAL          (1U << 5)
#define FAULT_3V3_LINE          (1U << 6)
#define FAULT_ADC               (1U << 7)

void fault_pin_init(void);
uint16_t fault_get(void);
uint8_t fault_active(void);
void fault_set(uint16_t fault);
void fault_clear(uint16_t fault);

#endif // FAULT_H