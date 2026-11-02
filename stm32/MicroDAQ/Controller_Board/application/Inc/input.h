
#ifndef INPUT_H
#define INPUT_H

#include <stdint.h>
#include "stm32h5xx.h"

#define BTN_PIN 13U //PC13

extern volatile uint8_t btn1_debounce_ticks;
extern volatile uint8_t btn1_debounce_active;
extern volatile uint8_t btn1_debounce_done;
extern volatile uint8_t btn1_stable_state;
extern volatile uint32_t time_ms;

void gpio_input_init(GPIO_TypeDef *GPIOx, uint32_t pin);
void btn1_changed_callback(void);

#endif // INPUT_H