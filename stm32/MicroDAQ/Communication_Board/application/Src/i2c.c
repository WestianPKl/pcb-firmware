#include "i2c.h"
#include "stm32c542xx.h"
#include <stddef.h>

static i2c_config_t current_config;

static uint8_t i2c1_address_get(uint16_t addr, uint32_t *sadd)
{
    if (sadd == NULL)
    {
        return 0U;
    }

    if (current_config.addressing_mode == 0U)
    {
        if (addr > 0x7FU)
        {
            return 0U;
        }

        *sadd = ((uint32_t)addr << 1U) & I2C_CR2_SADD_Msk;
    }
    else
    {
        if (addr > 0x3FFU)
        {
            return 0U;
        }

        *sadd = (uint32_t)addr & I2C_CR2_SADD_Msk;
    }

    return 1U;
}

void i2c1_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN | RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    RCC->CCIPR2 &= ~RCC_CCIPR2_I2C1SEL_Msk;

    GPIOA->MODER &= ~(0x3U << (I2C1_SCL * 2U));
    GPIOB->MODER &= ~(0x3U << (I2C1_SDA * 2U));
    GPIOA->MODER |= (0x2U << (I2C1_SCL * 2U));
    GPIOB->MODER |= (0x2U << (I2C1_SDA * 2U));

    GPIOA->OTYPER |= (0x1U << I2C1_SCL);
    GPIOB->OTYPER |= (0x1U << I2C1_SDA);
    GPIOA->OSPEEDR |= (0x3U << (I2C1_SCL * 2U));
    GPIOB->OSPEEDR |= (0x3U << (I2C1_SDA * 2U));
    GPIOA->PUPDR &= ~(0x3U << (I2C1_SCL * 2U));
    GPIOB->PUPDR &= ~(0x3U << (I2C1_SDA * 2U));

    GPIOA->AFR[1] &= ~(0xFU << ((I2C1_SCL - 8U) * 4U));
    GPIOA->AFR[1] |= (0x9U << ((I2C1_SCL - 8U) * 4U));

    GPIOB->AFR[0] &= ~(0xFU << (I2C1_SDA * 4U));
    GPIOB->AFR[0] |= (0x4U << (I2C1_SDA * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_I2C1EN;
    (void)RCC->APB1LENR;

    I2C1->TIMINGR = 0xB0420F13U;

    current_config.clock_frequency = 100000U;
    current_config.addressing_mode = 0U;

    I2C1->CR1 |= (0x1U << I2C_CR1_PE_Pos);
}

uint8_t i2c1_config(const i2c_config_t *config)
{
    if (config == NULL) return 0U;
    if (config->clock_frequency == 0U) return 0U;
    if (config->addressing_mode != 0U && config->addressing_mode != 1U) return 0U;

    uint32_t timing;

    switch(config->clock_frequency)
    {
        case 100000U:
            timing = 0xB0420F13U;
            break;
        case 400000U:
            timing = 0x50330309U;
            break;
        default:
            return 0U;
    }

    I2C1->CR1 &= ~I2C_CR1_PE_Msk;
    I2C1->TIMINGR = timing;

    I2C1->CR1 |= (0x1U << I2C_CR1_PE_Pos);

    current_config = *config;
    return 1U;
}

static uint8_t i2c1_stop_done(void)
{
    uint32_t t = 100000U;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { if (!--t) return 0; }
    uint8_t ok = (I2C1->ISR & I2C_ISR_NACKF) ? 0U : 1U;
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    return ok;
}

uint8_t i2c1_write(uint16_t addr, const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 255U))
    {
        return 0U;
    }

    uint32_t sadd;

    if (i2c1_address_get(addr, &sadd) == 0U)
    {
        return 0U;
    }

    uint32_t t = 100000U;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    uint32_t cr2 = 0U;

    cr2 |= sadd;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_Pos);
    cr2 |= I2C_CR2_AUTOEND_Msk;

    if (current_config.addressing_mode != 0U)
    {
        cr2 |= I2C_CR2_ADD10_Msk;
    }

    I2C1->CR2 = cr2;
    I2C1->CR2 |= I2C_CR2_START_Msk;

    for (uint16_t i = 0U; i < len; i++)
    {
        t = 100000U;

        while ((I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)) == 0U)
        {
            if (--t == 0U)
            {
                return 0U;
            }
        }

        if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
        {
            return i2c1_stop_done();
        }

        I2C1->TXDR = buf[i];
    }

    return i2c1_stop_done();
}

uint8_t i2c1_read(uint16_t addr, uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 255U))
    {
        return 0U;
    }

    uint32_t sadd;

    if (i2c1_address_get(addr, &sadd) == 0U)
    {
        return 0U;
    }

    uint32_t t = 100000U;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    uint32_t cr2 = 0U;

    cr2 |= sadd;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_Pos);
    cr2 |= I2C_CR2_RD_WRN_Msk;
    cr2 |= I2C_CR2_AUTOEND_Msk;

    if (current_config.addressing_mode != 0U)
    {
        cr2 |= I2C_CR2_ADD10_Msk;
    }

    I2C1->CR2 = cr2;
    I2C1->CR2 |= I2C_CR2_START_Msk;

    for (uint16_t i = 0U; i < len; i++)
    {
        t = 100000U;

        while ((I2C1->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF)) == 0U)
        {
            if (--t == 0U)
            {
                return 0U;
            }
        }

        if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
        {
            return i2c1_stop_done();
        }

        buf[i] = (uint8_t)I2C1->RXDR;
    }

    return i2c1_stop_done();
}

uint8_t i2c1_write_reg(uint16_t addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 254U))
    {
        return 0U;
    }

    uint32_t sadd;

    if (i2c1_address_get(addr, &sadd) == 0U)
    {
        return 0U;
    }

    uint32_t t = 100000U;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    uint32_t cr2 = 0U;

    cr2 |= sadd;
    cr2 |= ((uint32_t)(len + 1U) << I2C_CR2_NBYTES_Pos);
    cr2 |= I2C_CR2_AUTOEND_Msk;

    if (current_config.addressing_mode != 0U)
    {
        cr2 |= I2C_CR2_ADD10_Msk;
    }

    I2C1->CR2 = cr2;
    I2C1->CR2 |= I2C_CR2_START_Msk;

    t = 100000U;

    while ((I2C1->ISR &
           (I2C_ISR_TXIS | I2C_ISR_NACKF)) == 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
    {
        return i2c1_stop_done();
    }

    I2C1->TXDR = reg;

    for (uint16_t i = 0U; i < len; i++)
    {
        t = 100000U;

        while ((I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)) == 0U)
        {
            if (--t == 0U)
            {
                return 0U;
            }
        }

        if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
        {
            return i2c1_stop_done();
        }

        I2C1->TXDR = buf[i];
    }

    return i2c1_stop_done();
}

uint8_t i2c1_read_reg(uint16_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 255U))
    {
        return 0U;
    }

    uint32_t sadd;

    if (i2c1_address_get(addr, &sadd) == 0U)
    {
        return 0U;
    }

    uint32_t t = 100000U;

    while ((I2C1->ISR & I2C_ISR_BUSY) != 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    uint32_t cr2 = 0U;

    cr2 |= sadd;
    cr2 |= (1U << I2C_CR2_NBYTES_Pos);

    if (current_config.addressing_mode != 0U)
    {
        cr2 |= I2C_CR2_ADD10_Msk;
    }

    I2C1->CR2 = cr2;
    I2C1->CR2 |= I2C_CR2_START_Msk;

    t = 100000U;

    while ((I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF)) == 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
    {
        return i2c1_stop_done();
    }

    I2C1->TXDR = reg;

    t = 100000U;

    while ((I2C1->ISR & (I2C_ISR_TC | I2C_ISR_NACKF)) == 0U)
    {
        if (--t == 0U)
        {
            return 0U;
        }
    }

    if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
    {
        return i2c1_stop_done();
    }

    cr2 = 0U;

    cr2 |= sadd;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_Pos);
    cr2 |= I2C_CR2_RD_WRN_Msk;
    cr2 |= I2C_CR2_AUTOEND_Msk;

    if (current_config.addressing_mode != 0U)
    {
        cr2 |= I2C_CR2_ADD10_Msk;
        cr2 |= I2C_CR2_HEAD10R_Msk;
    }

    I2C1->CR2 = cr2;
    I2C1->CR2 |= I2C_CR2_START_Msk;

    for (uint16_t i = 0U; i < len; i++)
    {
        t = 100000U;

        while ((I2C1->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF)) == 0U)
        {
            if (--t == 0U)
            {
                return 0U;
            }
        }

        if ((I2C1->ISR & I2C_ISR_NACKF) != 0U)
        {
            return i2c1_stop_done();
        }

        buf[i] = (uint8_t)I2C1->RXDR;
    }

    return i2c1_stop_done();
}