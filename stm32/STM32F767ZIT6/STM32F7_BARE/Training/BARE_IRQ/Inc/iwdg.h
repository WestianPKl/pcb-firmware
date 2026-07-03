#ifndef IWDG_H
#define IWDG_H

#include <stdint.h>
#include "stm32f7xx.h"

void iwdg_init(void);
void iwdg_refresh(void);

#endif // IWDG_H