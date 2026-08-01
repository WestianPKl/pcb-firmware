#include "tca9548.h"
#include "i2c.h"

#define TCA9548_ADDRESS 0x70

int tca9548_set_channel(uint8_t channel)
{
    if (channel > 7U) {
        return -1;
    }

    uint8_t data = (uint8_t)(1U << channel);
    return (i2c2_write(TCA9548_ADDRESS, &data, 1) == 1U) ? 1 : -1;
}

int tca9548_disable_all(void)
{
    uint8_t data = 0;
    return (i2c2_write(TCA9548_ADDRESS, &data, 1) == 1U) ? 1 : -1;
}

int tca9548_get_channel(uint8_t *channel)
{
    if (!channel) {
        return -1;
    }

    uint8_t data = 0;
    if (i2c2_read(TCA9548_ADDRESS, &data, 1) != 1U) {
        return -1;
    }

    *channel = data;
    return 1;
}