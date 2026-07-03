#include "rng.h"

uint8_t rng_get(uint32_t *out)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_RNGEN;
    (void)RCC->AHB2ENR;

    RNG->CR |= RNG_CR_RNGEN;

    if (RNG->SR & (RNG_SR_CECS | RNG_SR_SECS)) {
        RNG->CR &= ~RNG_CR_RNGEN;
        RNG->CR |=  RNG_CR_RNGEN;
        return 0;
    }

    uint32_t t = 100000U;
    while (!(RNG->SR & RNG_SR_DRDY) && --t);
    if (t == 0U) return 0;

    *out = RNG->DR;
    return 1;
}
