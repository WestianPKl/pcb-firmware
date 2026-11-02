#include "i2c.h"
#include <stddef.h>


void i2c1_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~((3U << (I2C1_SDA * 2)) | (3U << (I2C1_SCL * 2)));
    GPIOB->MODER |= (2U << (I2C1_SDA * 2)) | (2U << (I2C1_SCL * 2));
    GPIOB->OTYPER |= (1U << I2C1_SDA) | (1U << I2C1_SCL);
    GPIOB->OSPEEDR |= (3U << (I2C1_SDA * 2)) | (3U << (I2C1_SCL * 2));
    GPIOB->PUPDR &= ~((3U << (I2C1_SDA * 2)) | (3U << (I2C1_SCL * 2)));
    GPIOB->PUPDR |= (1U << (I2C1_SDA * 2)) | (1U << (I2C1_SCL * 2));
    
    GPIOB->AFR[1] &= ~((0xFU << ((I2C1_SDA - 8U) * 4U)) | (0xFU << ((I2C1_SCL - 8U) * 4U)));
    GPIOB->AFR[1] |= (4U << ((I2C1_SDA - 8U) * 4U)) | (4U << ((I2C1_SCL - 8U) * 4U));

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    I2C1->TIMINGR = 0x20404768U;

    I2C1->CR1 |= (0x1U << I2C_CR1_PE_Pos);
}

static uint8_t i2c1_stop_done(void)
{
    uint32_t t = 100000U;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { if (!--t) return 0; }
    uint8_t ok = (I2C1->ISR & I2C_ISR_NACKF) ? 0U : 1U;
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    return ok;
}

uint8_t i2c1_write(uint8_t addr, const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 255U))
    {
        return 0U;
    }

    uint32_t t = 100000U;

    while (I2C1->ISR & I2C_ISR_BUSY)
    {
        if (!--t)
        {
            return 0U;
        }
    }

    I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        t = 100000U;
        while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C1->ISR & I2C_ISR_NACKF) break;
        I2C1->TXDR = buf[i];
    }
    return i2c1_stop_done();
}


uint8_t i2c1_read(uint8_t addr, uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 255U)) return 0;

    uint32_t t = 100000U;

    while (I2C1->ISR & I2C_ISR_BUSY)
    {
        if (!--t)
        {
            return 0U;
        }
    }

    I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        t = 100000U;
        while (!(I2C1->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C1->ISR & I2C_ISR_NACKF) break;
        buf[i] = (uint8_t)(I2C1->RXDR & 0xFFU);
    }
    return i2c1_stop_done();
}

uint8_t i2c1_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 254U)) return 0;

    uint32_t t = 100000U;

    while (I2C1->ISR & I2C_ISR_BUSY)
    {
        if (!--t)
        {
            return 0U;
        }
    }

    I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | ((((uint32_t)len + 1U) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    t = 100000U;
    while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
    if (I2C1->ISR & I2C_ISR_NACKF) return i2c1_stop_done();
    I2C1->TXDR = reg;

    for (uint16_t i = 0; i < len; i++) {
        t = 100000U;
        while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C1->ISR & I2C_ISR_NACKF) break;
        I2C1->TXDR = buf[i];
    }
    return i2c1_stop_done();
}

uint8_t i2c1_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if ((buf == NULL) || (len == 0U) || (len > 255U)) return 0;

    uint32_t t = 100000U;

    while (I2C1->ISR & I2C_ISR_BUSY)
    {
        if (!--t)
        {
            return 0U;
        }
    }

    I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | ((uint32_t)1U << I2C_CR2_NBYTES_Pos);
    I2C1->CR2 |= I2C_CR2_START;

    t = 100000U;
    while (!(I2C1->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
    if (I2C1->ISR & I2C_ISR_NACKF) return i2c1_stop_done();
    I2C1->TXDR = reg;

    t = 100000U;
    while (!(I2C1->ISR & (I2C_ISR_TC | I2C_ISR_NACKF))) { if (!--t) return 0; }
    if (I2C1->ISR & I2C_ISR_NACKF) return i2c1_stop_done();

    I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    I2C1->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        t = 100000U;
        while (!(I2C1->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C1->ISR & I2C_ISR_NACKF) break;
        buf[i] = (uint8_t)(I2C1->RXDR & 0xFFU);
    }
    return i2c1_stop_done();
}