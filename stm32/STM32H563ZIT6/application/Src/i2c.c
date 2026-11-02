#include "i2c.h"
#include "support.h"

// volatile uint8_t i2c1_tx_dma_busy = 0U;
// volatile uint8_t i2c1_tx_dma_done = 0U;
// volatile uint8_t i2c1_tx_dma_error = 0U;

// volatile uint8_t i2c1_rx_dma_busy = 0U;
// volatile uint8_t i2c1_rx_dma_done = 0U;
// volatile uint8_t i2c1_rx_dma_error = 0U;

volatile uint8_t i2c2_received;

// static void i2c1_write_dma_init(void)
// {
//     RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
//     (void)RCC->AHB1ENR;

//     GPDMA1_Channel2->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

//     GPDMA1_Channel2->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
//     GPDMA1_Channel2->CTR1 |= (0x1U << DMA_CTR1_SINC_Pos);

//     GPDMA1_Channel2->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
//     GPDMA1_Channel2->CTR2 |= (0xDU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_DREQ_Pos);

//     GPDMA1_Channel2->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
//     GPDMA1_Channel2->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

//     NVIC->IPR[29] = 0x50U;
//     NVIC->ICPR[0] = (1U << 29U);
//     NVIC->ISER[0] = (1U << 29U);

//     I2C1->CR1 |= (0x1U << I2C_CR1_TXDMAEN_Pos);
// }

// static void i2c1_read_dma_init(void)
// {
//     RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
//     (void)RCC->AHB1ENR;

//     GPDMA1_Channel3->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos));

//     GPDMA1_Channel3->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
//     GPDMA1_Channel3->CTR1 |= (0x1U << DMA_CTR1_DINC_Pos);

//     GPDMA1_Channel3->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_DREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
//     GPDMA1_Channel3->CTR2 |= (0xCU << DMA_CTR2_REQSEL_Pos);

//     GPDMA1_Channel3->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos);
//     GPDMA1_Channel3->CCR |= (0x1U << DMA_CCR_DTEIE_Pos);

//     NVIC->IPR[30] = 0x50U;
//     NVIC->ICPR[0] = (1U << 30U);
//     NVIC->ISER[0] = (1U << 30U);

//     I2C1->CR1 |= (0x1U << I2C_CR1_RXDMAEN_Pos);
// }

void i2c1_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    GPIOB->MODER &= ~(0x3U << (I2C1_SCL * 2U) | 0x3U << (I2C1_SDA * 2U));
    GPIOB->MODER |= (0x2U << (I2C1_SCL * 2U) | 0x2U << (I2C1_SDA * 2U));

    GPIOB->OTYPER |= (0x1U << I2C1_SCL) | (0x1U << I2C1_SDA);
    GPIOB->OSPEEDR |= (0x3U << (I2C1_SCL * 2U) | 0x3U << (I2C1_SDA * 2U));
    GPIOB->PUPDR &= ~(0x3U << (I2C1_SCL * 2U) | 0x3U << (I2C1_SDA * 2U));

    GPIOB->AFR[1] &= ~((0xFU << ((I2C1_SCL - 8U) * 4U)) | (0xFU << ((I2C1_SDA - 8U) * 4U)));
    GPIOB->AFR[1] |= (0x4U << ((I2C1_SCL - 8U) * 4U)) | (0x4U << ((I2C1_SDA - 8U) * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_I2C1EN;
    (void)RCC->APB1LENR;

    I2C1->TIMINGR = 0x80926D9EU;

    // i2c1_write_dma_init();
    // i2c1_read_dma_init();

    // I2C1->CR1 |= I2C_CR1_STOPIE;

    // NVIC->IPR[51] = 0x50U;
    // NVIC->ICPR[1] = (1U << 19U);
    // NVIC->ISER[1] = (1U << 19U);

    I2C1->CR1 |= (0x1U << I2C_CR1_PE_Pos);
}

void i2c2_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOFEN;
    (void)RCC->AHB2ENR;

    GPIOF->MODER &= ~(0x3U << (I2C2_SCL * 2U) | 0x3U << (I2C2_SDA * 2U));
    GPIOF->MODER |= (0x2U << (I2C2_SCL * 2U) | 0x2U << (I2C2_SDA * 2U));

    GPIOF->OTYPER |= (0x1U << I2C2_SCL) | (0x1U << I2C2_SDA);
    GPIOF->OSPEEDR |= (0x3U << (I2C2_SCL * 2U) | 0x3U << (I2C2_SDA * 2U));
    GPIOF->PUPDR &= ~(0x3U << (I2C2_SCL * 2U) | 0x3U << (I2C2_SDA * 2U));

    GPIOF->AFR[0] &= ~((0xFU << (I2C2_SCL * 4U)) | (0xFU << (I2C2_SDA * 4U)));
    GPIOF->AFR[0] |= (0x4U << (I2C2_SCL * 4U)) | (0x4U << (I2C2_SDA * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_I2C2EN;
    (void)RCC->APB1LENR;

    I2C2->TIMINGR = 0x80926D9EU;

    I2C2->CR1 &= ~(I2C_CR1_PE_Msk);

    I2C2->OAR1 &= ~(I2C_OAR1_OA1EN_Msk | I2C_OAR1_OA1MODE_Msk | I2C_OAR1_OA1_Msk); 
    I2C2->OAR1 |= ((0x32U << 1U) & I2C_OAR1_OA1_Msk) | I2C_OAR1_OA1EN_Msk;

    I2C2->CR1 |= (0x1U << I2C_CR1_NACKIE_Pos) | (0x1U << I2C_CR1_STOPIE_Pos | (0x1U << I2C_CR1_ADDRIE_Pos));

    NVIC->IPR[53] = 0x50U;
    NVIC->ICPR[1] = (1U << 21U);
    NVIC->ISER[1] = (1U << 21U);

    I2C2->CR1 |= (0x1U << I2C_CR1_PE_Pos);
}

static uint8_t i2c1_stop_done(void)
{
    uint32_t t = 100000U;
    while (!(I2C1->ISR & I2C_ISR_STOPF)) { if (!--t) return 0; }
    uint8_t ok = (I2C1->ISR & I2C_ISR_NACKF) ? 0U : 1U;
    I2C1->ICR = I2C_ICR_STOPCF | I2C_ICR_NACKCF;
    return ok;
}

// uint8_t i2c1_dma_write_start(uint8_t addr, const uint8_t *buf, uint16_t len)
// {
//     if ((buf == NULL) || (len == 0U) || (len > 255U))
//     {
//         return 0U;
//     }

//     if ((i2c1_tx_dma_busy != 0U) ||
//         (i2c1_rx_dma_busy != 0U) ||
//         ((I2C1->ISR & I2C_ISR_BUSY) != 0U))
//     {
//         return 0U;
//     }

//     if ((GPDMA1_Channel2->CCR & (1U << DMA_CCR_EN_Pos)) != 0U)
//     {
//         return 0U;
//     }

//     i2c1_tx_dma_busy = 1U;
//     i2c1_tx_dma_done = 0U;
//     i2c1_tx_dma_error = 0U;

//     GPDMA1_Channel2->CSAR = (uint32_t)buf;
//     GPDMA1_Channel2->CDAR = (uint32_t)&I2C1->TXDR;
//     GPDMA1_Channel2->CBR1 = len;

//     GPDMA1_Channel2->CFCR = (1U << DMA_CFCR_TCF_Pos) | (1U << DMA_CFCR_DTEF_Pos);
//     GPDMA1_Channel2->CCR |= (1U << DMA_CCR_EN_Pos);

//     I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
//               | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
//               | I2C_CR2_AUTOEND;
//     I2C1->CR2 |= I2C_CR2_START;

//     return 1U;
// }

// uint8_t i2c1_dma_read_start(uint8_t addr, uint8_t *buf, uint16_t len)
// {
//     if ((buf == NULL) || (len == 0U) || (len > 255U))
//     {
//         return 0U;
//     }

//     if ((i2c1_tx_dma_busy != 0U) ||
//         (i2c1_rx_dma_busy != 0U) ||
//         ((I2C1->ISR & I2C_ISR_BUSY) != 0U))
//     {
//         return 0U;
//     }

//     if ((GPDMA1_Channel3->CCR & (1U << DMA_CCR_EN_Pos)) != 0U)
//     {
//         return 0U;
//     }

//     i2c1_rx_dma_busy = 1U;
//     i2c1_rx_dma_done = 0U;
//     i2c1_rx_dma_error = 0U;

//     GPDMA1_Channel3->CSAR = (uint32_t)&I2C1->RXDR;
//     GPDMA1_Channel3->CDAR = (uint32_t)buf;
//     GPDMA1_Channel3->CBR1 = len;

//     GPDMA1_Channel3->CFCR = (1U << DMA_CFCR_TCF_Pos) | (1U << DMA_CFCR_DTEF_Pos);
//     GPDMA1_Channel3->CCR |= (1U << DMA_CCR_EN_Pos);

//     I2C1->CR2 = (((uint32_t)addr << 1U) & I2C_CR2_SADD)
//               | (((uint32_t)len << I2C_CR2_NBYTES_Pos) & I2C_CR2_NBYTES)
//               | I2C_CR2_RD_WRN | I2C_CR2_AUTOEND;
//     I2C1->CR2 |= I2C_CR2_START;

//     return 1U;
// }

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

// void I2C1_EV_IRQHandler(void)
// {
//     uint32_t isr = I2C1->ISR;

//     if ((isr & I2C_ISR_STOPF) != 0U)
//     {
//         I2C1->ICR = I2C_ICR_STOPCF;

//         if (i2c1_tx_dma_busy != 0U)
//         {
//             i2c1_tx_dma_busy = 0U;
//             i2c1_tx_dma_done = 1U;
//         }

//         if (i2c1_rx_dma_busy != 0U)
//         {
//             i2c1_rx_dma_busy = 0U;
//             i2c1_rx_dma_done = 1U;
//         }
//     }
// }

void I2C2_EV_IRQHandler(void)
{
    uint32_t isr = I2C2->ISR;

    if ((isr & I2C_ISR_ADDR) != 0U)
    {
        if ((isr & I2C_ISR_DIR) != 0U)
        {
            I2C2->CR1 |= I2C_CR1_TXIE;
            I2C2->CR1 &= ~I2C_CR1_RXIE;
        }
        else
        {
            I2C2->CR1 |= I2C_CR1_RXIE;
            I2C2->CR1 &= ~I2C_CR1_TXIE;
        }

        I2C2->ICR = I2C_ICR_ADDRCF;
    }

    if ((I2C2->ISR & I2C_ISR_RXNE) != 0U)
    {
        i2c2_received = (uint8_t)I2C2->RXDR;
    }

    if ((I2C2->ISR & I2C_ISR_TXIS) != 0U)
    {
        I2C2->TXDR = 0x56U;
    }

    if ((I2C2->ISR & I2C_ISR_NACKF) != 0U)
    {
        I2C2->ICR = I2C_ICR_NACKCF;
    }

    if ((I2C2->ISR & I2C_ISR_STOPF) != 0U)
    {
        I2C2->ICR = I2C_ICR_STOPCF;

        I2C2->CR1 &= ~(I2C_CR1_TXIE | I2C_CR1_RXIE);
    }
}

// void GPDMA1_CH2_IRQHandler(void)
// {
//     uint32_t status = GPDMA1_Channel2->CSR;

//     if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
//     {
//         GPDMA1_Channel2->CFCR = (1U << DMA_CFCR_DTEF_Pos);
//         i2c1_tx_dma_error = 1U;
//     }
// }

// void GPDMA1_CH3_IRQHandler(void)
// {
//     uint32_t status = GPDMA1_Channel3->CSR;

//     if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
//     {
//         GPDMA1_Channel3->CFCR = (1U << DMA_CFCR_DTEF_Pos);
//         i2c1_rx_dma_error = 1U;
//     }
// }