#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>
#include "stm32f7xx.h"

#define BTN_1 (1U << 13) // Button 1 PC13

extern volatile uint8_t btn_pressed;

void init_btn(void);
void btn1_handler(void);

#endif // INPUTS_H