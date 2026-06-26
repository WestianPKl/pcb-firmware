#include "support.h"

static uint32_t apb1_div_get(void)
{
    static const uint8_t apb_presc_table[8] = { 1, 1, 1, 1, 2, 4, 8, 16 };
    uint32_t ppre1_bits = (RCC->CFGR >> RCC_CFGR_PPRE1_Pos) & 0x07U;
    return apb_presc_table[ppre1_bits];
}

uint32_t apb1_pclk_hz(void)
{
    static const uint16_t ahb_presc_table[16] = {
        1,1,1,1,1,1,1,1, 2,4,8,16,64,128,256,512
    };
    uint32_t hpre_bits = (RCC->CFGR >> RCC_CFGR_HPRE_Pos) & 0x0FU;
    uint32_t hclk = SystemCoreClock / ahb_presc_table[hpre_bits];
    return hclk / apb1_div_get();
}

uint32_t tim_apb1_clock_hz(void)
{
    uint32_t pclk1 = apb1_pclk_hz();
    uint32_t div   = apb1_div_get();
    uint8_t  multiplier;

    if ((RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? (uint8_t)div : 4U;
    }
    return pclk1 * multiplier;
}


static uint32_t apb2_div_get(void)
{
    static const uint8_t apb_presc_table[8] = { 1, 1, 1, 1, 2, 4, 8, 16 };
    uint32_t ppre2_bits = (RCC->CFGR >> RCC_CFGR_PPRE2_Pos) & 0x07U;
    return apb_presc_table[ppre2_bits];
}

uint32_t apb2_pclk_hz(void)
{
    static const uint16_t ahb_presc_table[16] = {
        1,1,1,1,1,1,1,1, 2,4,8,16,64,128,256,512
    };
    uint32_t hpre_bits = (RCC->CFGR >> RCC_CFGR_HPRE_Pos) & 0x0FU;
    uint32_t hclk = SystemCoreClock / ahb_presc_table[hpre_bits];
    return hclk / apb2_div_get();
}

uint32_t tim_apb2_clock_hz(void)
{
    uint32_t pclk2 = apb2_pclk_hz();
    uint32_t div   = apb2_div_get();
    uint8_t  multiplier;

    if ((RCC->DCKCFGR1 & RCC_DCKCFGR1_TIMPRE) == 0U) {
        multiplier = (div == 1U) ? 1U : 2U;
    } else {
        multiplier = (div <= 4U) ? (uint8_t)div : 4U;
    }
    return pclk2 * multiplier;
}


void set_ahb1_periph_clock(uint32_t periph)
{
    SET_BIT(RCC->AHB1ENR, periph);
    (void)RCC->AHB1ENR; // delay after enabling clock
}

void set_ahb2_periph_clock(uint32_t periph)
{
    SET_BIT(RCC->AHB2ENR, periph);
    (void)RCC->AHB2ENR; // delay after enabling clock
}

void set_apb1_periph_clock(uint32_t periph)
{
    SET_BIT(RCC->APB1ENR, periph);
    (void)RCC->APB1ENR; // delay after enabling clock
}


void set_apb2_periph_clock(uint32_t periph)
{
    SET_BIT(RCC->APB2ENR, periph);
    (void)RCC->APB2ENR; // delay after enabling clock
}

void set_pin_mode(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t mode)
{
    MODIFY_REG(GPIOx->MODER, (0x3U << (pin * 2U)), (mode << (pin * 2U)));
}

void set_pin_output_type(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t type)
{
    MODIFY_REG(GPIOx->OTYPER, (0x1U << pin), (type << pin));
}

void set_pin_output_speed(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t speed)
{
    MODIFY_REG(GPIOx->OSPEEDR, (0x3U << (pin * 2U)), (speed << (pin * 2U)));
}

void set_pin_pull(GPIO_TypeDef *GPIOx, uint32_t pin, uint32_t pull)
{
    MODIFY_REG(GPIOx->PUPDR, (0x3U << (pin * 2U)), (pull << (pin * 2U)));
}