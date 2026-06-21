#ifndef INPUTS_H
#define INPUTS_H

#include <stdint.h>
#include "stm32f7xx.h"

// BTN1 PC13
#define BTN_1 (1U << 13)

void init_btn(void);
void btn1_handler(void);

#endif // INPUTS_H