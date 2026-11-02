#include "i2c.h"
#include "support.h"
#include "dma.h"

volatile uint8_t i2c1_rx_buffer[I2C1_BUF_SIZE];
volatile uint8_t i2c1_tx_buffer[I2C1_BUF_SIZE];
volatile uint8_t i2c1_rx_idx;
volatile uint8_t i2c1_tx_idx;
volatile uint8_t i2c1_rx_done;

static void i2c1_set_address(uint32_t address)
{
    I2C1->OAR1 = (address << 1U) | 0;
}


void i2c1_slave_config(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~((3U << (I2C1_SCL * 2U)) | (3U << (I2C1_SDA * 2U)));
    GPIOB->MODER |= ((2U << (I2C1_SCL * 2U)) | (2U << (I2C1_SDA * 2U)));
    GPIOB->AFR[1] &= ~(0xFU << (0U * 4U));
    GPIOB->AFR[1] |= (0x4U << (0U * 4U));
    GPIOB->AFR[1] &= ~(0xFU << (1U * 4U));
    GPIOB->AFR[1] |= (0x4U << (1U * 4U));
    GPIOB->OTYPER |= (1U << I2C1_SCL) |(1U << I2C1_SDA);
    GPIOB->PUPDR &= ~((0x3U << (I2C1_SCL * 2U)) | (0x3U << (I2C1_SDA * 2U)));
    GPIOB->PUPDR |= (0x1U << (I2C1_SCL * 2U))  | (0x1U << (I2C1_SDA * 2U));

    RCC->APB1ENR |= RCC_APB1ENR_I2C1EN;
    (void)RCC->APB1ENR;

    NVIC_SetPriority(I2C1_EV_IRQn, 1);
    NVIC_EnableIRQ(I2C1_EV_IRQn);

    I2C1->CR1 &= ~I2C_CR1_PE;

    i2c1_set_address(I2C1_SLAVE_ADDRESS);

    I2C1->OAR1 |= I2C_OAR1_OA1EN;

    I2C1->CR1 |= I2C_CR1_PE;

    I2C1->CR1 |= I2C_CR1_ADDRIE;
    I2C1->CR1 |= I2C_CR1_NACKIE;
    I2C1->CR1 |= I2C_CR1_STOPIE;
}


void i2c2_master_config(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;
    
    GPIOB->MODER &= ~((3U << (I2C2_SCL * 2U)) | (3U << (I2C2_SDA * 2U)));
    GPIOB->MODER |= ((2U << (I2C2_SCL * 2U)) | (2U << (I2C2_SDA * 2U)));
    GPIOB->AFR[1] &= ~(0xFU << (2U * 4U));
    GPIOB->AFR[1] |= (0x4U << (2U * 4U));
    GPIOB->AFR[1] &= ~(0xFU << (3U * 4U));
    GPIOB->AFR[1] |= (0x4U << (3U * 4U));
    GPIOB->OTYPER |= (1U << I2C2_SCL) | (1U << I2C2_SDA);
    GPIOB->PUPDR &= ~((0x3U << (I2C2_SCL * 2U)) | (0x3U << (I2C2_SDA * 2U)));
    GPIOB->PUPDR |= (0x1U << (I2C2_SCL * 2U)) | (0x1U << (I2C2_SDA * 2U));

    RCC->APB1ENR |= RCC_APB1ENR_I2C2EN;
    (void)RCC->APB1ENR;

    I2C2->CR1 &= ~I2C_CR1_PE;

    I2C2->TIMINGR = I2C_TIMING;

    I2C2->CR1 |= I2C_CR1_TXDMAEN | I2C_CR1_RXDMAEN;

    I2C2->CR1 |= I2C_CR1_PE;

    i2c2_dma_tx_init();
    i2c2_dma_rx_init();
}

void i2c2_master_write(uint8_t addr, uint8_t *buf, uint16_t len)
{
    i2c2_dma_tx_start((uint32_t)buf, len);

    uint32_t cr2 = ((uint32_t)addr << 1U) & I2C_CR2_SADD;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES;
    cr2 |= I2C_CR2_AUTOEND;
    I2C2->CR2 = cr2;
    I2C2->CR2 |= I2C_CR2_START;
}

void i2c2_master_read(uint8_t addr, uint8_t *buf, uint16_t len)
{
    i2c2_dma_rx_start((uint32_t)buf, len);

    uint32_t cr2 = ((uint32_t)addr << 1U) & I2C_CR2_SADD;
    cr2 |= ((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES;
    cr2 |= I2C_CR2_AUTOEND | I2C_CR2_RD_WRN;
    I2C2->CR2 = cr2;
    I2C2->CR2 |= I2C_CR2_START;
}

#define I2C2_TO 100000U

static uint8_t i2c2_stop_done(void)
{
    uint32_t t = I2C2_TO;
    while (!(I2C2->ISR & I2C_ISR_STOPF)) { if (!--t) return 0; }
    uint8_t ok = (I2C2->ISR & I2C_ISR_NACKF) ? 0U : 1U;
    I2C2->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    return ok;
}

uint8_t i2c2_master_wait(void)
{
    return i2c2_stop_done();
}

uint8_t i2c2_write(uint8_t addr, const uint8_t *buf, uint16_t len)
{
    if (len > I2C2_MAX_NBYTES) return 0;

    I2C2->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_AUTOEND;
    I2C2->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        uint32_t t = I2C2_TO;
        while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C2->ISR & I2C_ISR_NACKF) break;
        I2C2->TXDR = buf[i];
    }
    return i2c2_stop_done();
}

uint8_t i2c2_read(uint8_t addr, uint8_t *buf, uint16_t len)
{
    if (len > I2C2_MAX_NBYTES) return 0;

    I2C2->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    I2C2->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        uint32_t t = I2C2_TO;
        while (!(I2C2->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C2->ISR & I2C_ISR_NACKF) break;
        buf[i] = (uint8_t)(I2C2->RXDR & 0xFFU);
    }
    return i2c2_stop_done();
}

uint8_t i2c2_write_reg(uint8_t addr, uint8_t reg, const uint8_t *buf, uint16_t len)
{
    if (len > I2C2_MAX_NBYTES_REG) return 0;

    I2C2->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | ((((uint32_t)len + 1U) << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_AUTOEND;
    I2C2->CR2 |= I2C_CR2_START;

    uint32_t t = I2C2_TO;
    while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
    if (I2C2->ISR & I2C_ISR_NACKF) return i2c2_stop_done();
    I2C2->TXDR = reg;

    for (uint16_t i = 0; i < len; i++) {
        t = I2C2_TO;
        while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C2->ISR & I2C_ISR_NACKF) break;
        I2C2->TXDR = buf[i];
    }
    return i2c2_stop_done();
}

uint8_t i2c2_read_reg(uint8_t addr, uint8_t reg, uint8_t *buf, uint16_t len)
{
    if (len > I2C2_MAX_NBYTES) return 0;

    I2C2->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | ((uint32_t)1U << I2C_CR2_NBYTES_Pos);
    I2C2->CR2 |= I2C_CR2_START;

    uint32_t t = I2C2_TO;
    while (!(I2C2->ISR & (I2C_ISR_TXIS | I2C_ISR_NACKF))) { if (!--t) return 0; }
    if (I2C2->ISR & I2C_ISR_NACKF) return i2c2_stop_done();
    I2C2->TXDR = reg;

    t = I2C2_TO;
    while (!(I2C2->ISR & (I2C_ISR_TC | I2C_ISR_NACKF))) { if (!--t) return 0; }
    if (I2C2->ISR & I2C_ISR_NACKF) return i2c2_stop_done();

    I2C2->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
              | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
              | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
    I2C2->CR2 |= I2C_CR2_START;

    for (uint16_t i = 0; i < len; i++) {
        t = I2C2_TO;
        while (!(I2C2->ISR & (I2C_ISR_RXNE | I2C_ISR_NACKF))) { if (!--t) return 0; }
        if (I2C2->ISR & I2C_ISR_NACKF) break;
        buf[i] = (uint8_t)(I2C2->RXDR & 0xFFU);
    }
    return i2c2_stop_done();
}

uint32_t i2c2_get_address_match_code(void)
{
    return (uint32_t)(READ_BIT(I2C2->ISR, I2C_ISR_ADDCODE) >> I2C_ISR_ADDCODE_Pos);
}

void I2C1_EV_IRQHandler(void)
{
    uint32_t isr = I2C1->ISR;

    if (isr & I2C_ISR_ADDR) 
    {
        i2c1_rx_idx = 0;
        i2c1_tx_idx = 0;
        I2C1->ICR = I2C_ICR_ADDRCF;
        I2C1->ISR |= I2C_ISR_TXE;
        I2C1->CR1 |= I2C_CR1_RXIE | I2C_CR1_TXIE;
    }

    if (isr & I2C_ISR_RXNE) 
    {
        uint8_t d = (uint8_t)(I2C1->RXDR & 0xFFU);
        if (i2c1_rx_idx < I2C1_BUF_SIZE) i2c1_rx_buffer[i2c1_rx_idx++] = d;
    }

    if (isr & I2C_ISR_TXIS) 
    {
        I2C1->TXDR = (i2c1_tx_idx < I2C1_BUF_SIZE) ? i2c1_tx_buffer[i2c1_tx_idx++] : 0xFFU;
    }

    if (isr & I2C_ISR_NACKF) 
    {
        I2C1->ICR = I2C_ICR_NACKCF;
    }

    if (isr & I2C_ISR_STOPF) {
        I2C1->ICR = I2C_ICR_STOPCF;
        I2C1->CR1 &= ~(I2C_CR1_RXIE | I2C_CR1_TXIE);
        i2c1_rx_done = 1;
    }
}