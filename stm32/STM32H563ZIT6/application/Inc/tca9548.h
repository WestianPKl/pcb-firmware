#ifndef TCA9548_H
#define TCA9548_H

#include <stdint.h>

int tca9548_set_channel(uint8_t channel);
int tca9548_disable_all(void);
int tca9548_get_channel(uint8_t *mask);

#endif // TCA9548_H