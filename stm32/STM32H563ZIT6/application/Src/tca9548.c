#include "tca9548.h"
#include "i2c.h"

#define TCA9548_ADDRESS 0x70

int tca9548_set_channel(uint8_t channel)
{
    if (channel > 7U) {
        return 0U;
    }

    uint8_t data = (uint8_t)(1U << channel);
    return (i2c1_write(TCA9548_ADDRESS, &data, 1) == 1U) ? 1U : 0U;
}

int tca9548_disable_all(void)
{
    uint8_t data = 0;
    return (i2c1_write(TCA9548_ADDRESS, &data, 1) == 1U) ? 1U : 0U;
}

int tca9548_get_channel(uint8_t *mask)
{
    if (!mask) {
        return 0U;
    }

    uint8_t data = 0;
    if (i2c1_read(TCA9548_ADDRESS, &data, 1) != 1U) {
        return 0U;
    }

    *mask = data;
    return 1U;
}