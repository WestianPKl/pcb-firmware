#include "i2c.h"
#include "support.h"

static void i2c2_set_address(uint32_t address, uint8_t size)
{
    MODIFY_REG(I2C2->OAR1, I2C_OAR1_OA1 | I2C_OAR1_OA1MODE, (address << 1U) | 0); // 7-bit addr in OA1[7:1]
}

#define I2C_TIMING 0x00303D5B // Example timing value for 100kHz I2C

// SCL: PB10
// SDA: PB11

void i2c2_slave_config(void)
{
    set_ahb1_periph_clock(RCC_AHB1ENR_GPIOBEN); // Enable GPIOB clock

    set_pin_mode(GPIOB, 10, 0x2U); // SCL
    GPIOB->AFR[1] &= ~(0xFU << (2U * 4U)); // Clear AF bits for PB10
    GPIOB->AFR[1] |= (0x4U << (2U * 4U)); // AF4 for PB10
    GPIOB->OTYPER |= (1U << 10U); // Open-drain for PB10 (required for I2C)
    GPIOB->PUPDR &= ~(0x3U << (10U * 2U)); // Clear pull-up/pull-down for PB10
    GPIOB->PUPDR |= (0x1U << (10U * 2U)); // Pull-up for PB10


    set_pin_mode(GPIOB, 11, 0x2U); // SDA
    GPIOB->AFR[1] &= ~(0xFU << (3U * 4U)); // Clear AF bits for PB11
    GPIOB->AFR[1] |= (0x4U << (3U * 4U)); // AF4 for PB11
    GPIOB->OTYPER |= (1U << 11U); // Open-drain for PB11 (required for I2C)
    GPIOB->PUPDR &= ~(0x3U << (11U * 2U)); // Clear pull-up/pull-down for PB11
    GPIOB->PUPDR |= (0x1U << (11U * 2U)); // Pull-up for PB11

    set_apb1_periph_clock(RCC_APB1ENR_I2C2EN); // Enable I2C2 clock

    NVIC_SetPriority(I2C2_EV_IRQn, 1); // Set I2C2 event interrupt priority
    NVIC_EnableIRQ(I2C2_EV_IRQn); // Enable I2C2 event

    I2C2->CR1 &= ~I2C_CR1_PE; // Disable I2C2 peripheral

    i2c2_set_address(I2C2_SLAVE_ADDRESS, 0); // Set slave address (7-bit)

    I2C2->OAR1 |= I2C_OAR1_OA1EN; // Enable own address 1

    I2C2->CR1 |= I2C_CR1_PE; // Enable I2C2 peripheral

    I2C2->CR1 |= I2C_CR1_ADDRIE; // Enable address match interrupt
    I2C2->CR1 |= I2C_CR1_NACKIE; // Enable NACK generation for slave receiver
    I2C2->CR1 |= I2C_CR1_STOPIE; // Enable STOP detection interrupt
}

// SCL: PB6
// SDA: PB9

void i2c1_master_config(void)
{
    set_ahb1_periph_clock(RCC_AHB1ENR_GPIOBEN); // Enable GPIOB clock
    
    set_pin_mode(GPIOB, 6, 0x2U); // SCL
    GPIOB->AFR[0] &= ~(0xFU << (6U * 4U)); // Clear AF bits for PB6
    GPIOB->AFR[0] |= (0x4U << (6U * 4U)); // AF4 for PB6
    GPIOB->OTYPER |= (1U << 6U); // Open-drain for PB6 (required for I2C)
    GPIOB->PUPDR &= ~(0x3U << (6U * 2U)); // Clear pull-up/pull-down for PB6
    GPIOB->PUPDR |= (0x1U << (6U * 2U)); // Pull-up for PB6


    set_pin_mode(GPIOB, 9, 0x2U); // SDA
    GPIOB->AFR[1] &= ~(0xFU << (1U * 4U)); // Clear AF bits for PB9
    GPIOB->AFR[1] |= (0x4U << (1U * 4U)); // AF4 for PB9
    GPIOB->OTYPER |= (1U << 9U); // Open-drain for PB9 (required for I2C)
    GPIOB->PUPDR &= ~(0x3U << (9U * 2U)); // Clear pull-up/pull-down for PB9
    GPIOB->PUPDR |= (0x1U << (9U * 2U)); // Pull-up for PB9

    set_apb1_periph_clock(RCC_APB1ENR_I2C1EN); // Enable I2C1 clock

    I2C1->CR1 &= ~I2C_CR1_PE; // Disable I2C1 peripheral

    I2C1->TIMINGR = I2C_TIMING; // Set timing for 100kHz I2C

    I2C1->CR1 |= I2C_CR1_PE; // Enable I2C1 peripheral
}

uint32_t i2c2_get_address_matchode(void)
{
    return (uint32_t)(READ_BIT(I2C2->ISR, I2C_ISR_ADDCODE) >> I2C_ISR_ADDCODE_Pos); // 7-bit matched address
}
