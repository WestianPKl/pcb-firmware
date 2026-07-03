#ifndef WWDG_H
#define WWDG_H

#include <stdint.h>
#include "stm32f7xx.h"

void wwdg_init(uint8_t prescaler, uint8_t window, uint8_t counter);
void wwdg_refresh(uint8_t counter);

#endif // WWDG_H