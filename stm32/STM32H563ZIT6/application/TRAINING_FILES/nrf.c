#include "nrf.h"
#include "spi.h"
#include "timer.h"

volatile uint8_t nrf_irq = 0U;

static inline void nrf_ce_high(void)
{
    GPIOF->BSRR = (1U << NRF_CE_PIN);
}

static inline void nrf_ce_low(void)
{
    GPIOF->BSRR = (1U << (NRF_CE_PIN + 16));
}

void init_pins(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOFEN;
    (void)RCC->AHB2ENR;

    GPIOF->MODER &= ~(0x3U << (NRF_IRQ_PIN * 2U));
    GPIOF->PUPDR &= ~(0x3U << (NRF_IRQ_PIN * 2U));
    GPIOF->PUPDR |= (0x1U << (NRF_IRQ_PIN * 2U));

    EXTI->IMR1 &= ~(0x1U << NRF_IRQ_PIN);
    
    EXTI->EXTICR[1] &= ~(0xFFU << 24U);
    EXTI->EXTICR[1] |=  (0x5U  << 24U);

    EXTI->RTSR1 &= ~(0x1U << NRF_IRQ_PIN);
    EXTI->FTSR1 &= ~(0x1U << NRF_IRQ_PIN);

    EXTI->FTSR1 |=  (1U << NRF_IRQ_PIN);
    EXTI->RPR1 = (0x1U << NRF_IRQ_PIN);
    EXTI->FPR1 = (0x1U << NRF_IRQ_PIN);

    NVIC->IPR[18] = 0x50U;
    NVIC->ICPR[0] = (1UL << 18U);
    NVIC->ISER[0] = (1UL << 18U);

    EXTI->IMR1 |= (0x1U << NRF_IRQ_PIN);

    GPIOF->MODER &= ~(0x3U << (NRF_CE_PIN * 2U));
    GPIOF->MODER |= (0x1U << (NRF_CE_PIN * 2U));
    GPIOF->OTYPER &= ~(0x1U << NRF_CE_PIN);
    GPIOF->OSPEEDR |= (0x3U << (NRF_CE_PIN * 2U));
    GPIOF->PUPDR &= ~(0x3U << (NRF_CE_PIN * 2U));
}

static uint8_t nrf_spi_transfer_dma(const uint8_t *tx, uint8_t *rx, uint16_t length)
{
    if (spi1_dma_transfer(tx, rx, length, 0U) == 0U)
    {
        return 0U;
    }

    while ((spi1_dma_done == 0U) && (spi1_dma_error == 0U)){}

    if (spi1_dma_error != 0U)
    {
        return 0U;
    }

    return 1U;
}


static uint8_t nrf_command(uint8_t command)
{
    uint8_t rx;

    return nrf_spi_transfer_dma(&command, &rx, 1U);
}


static uint8_t nrf_read_register(uint8_t reg, uint8_t *value)
{
    if (!value) return 0U;

    uint8_t txbuf[2];
    uint8_t rxbuf[2];

    txbuf[0] = CMD_R_REGISTER | (reg & 0x1FU);
    txbuf[1] = 0xFFU;

    if (nrf_spi_transfer_dma(txbuf, rxbuf, 2U) == 0U)
    {
        return 0U;
    }

    *value = rxbuf[1];

    return 1U;
}

static uint8_t nrf_write_register(uint8_t reg, uint8_t value)
{
    uint8_t txbuf[2];
    uint8_t rxbuf[2];

    txbuf[0] = CMD_W_REGISTER | (reg & 0x1FU);
    txbuf[1] = value;

    if (nrf_spi_transfer_dma(txbuf, rxbuf, 2U) == 0U)
    {
        return 0U;
    }

    return 1U;
}

static uint8_t nrf_write_register_multi(uint8_t reg,
                                 const uint8_t *data,
                                 uint8_t length)
{
    if ((data == NULL) || (length == 0U))
    {
        return 0U;
    }

    uint8_t txbuf[6];
    uint8_t rxbuf[6];

    txbuf[0] = CMD_W_REGISTER | (reg & 0x1FU);

    for (uint8_t i = 0U; i < length; i++)
    {
        txbuf[i + 1U] = data[i];
    }

    return nrf_spi_transfer_dma(txbuf, rxbuf, length + 1U);
}

static uint8_t nrf_set_rx_mode(void)
{
    uint8_t config;

    nrf_ce_low();

    if (nrf_read_register(REG_CONFIG, &config) == 0U)
    {
        return 0U;
    }

    config |= (1U << CONFIG_PRIM_RX);

    if (nrf_write_register(REG_CONFIG, config) == 0U)
    {
        return 0U;
    }

    nrf_ce_high();

    delay_10us(15U);
    return 1U;
}

static uint8_t nrf_set_tx_mode(void)
{
    uint8_t config;

    nrf_ce_low();

    if (nrf_read_register(REG_CONFIG, &config) == 0U)
    {
        return 0U;
    }

    config &= ~(1U << CONFIG_PRIM_RX);

    return nrf_write_register(REG_CONFIG, config);
}

uint8_t nrf_init(void)
{
    init_pins();
    nrf_ce_low();
    nrf_write_register(REG_CONFIG, (1U << CONFIG_EN_CRC) | (0U << CONFIG_CRCO) | (1U << CONFIG_PWR_UP) | (1U << CONFIG_PRIM_RX));
    nrf_write_register(REG_RF_SETUP, (0x3U << 1U));
    nrf_write_register(REG_SETUP_RETR, (0x7U << 0U));
    nrf_write_register(REG_DYNPD, 0U);
    nrf_write_register(REG_RF_CH, (0x7U << 0U));
    nrf_write_register(REG_RX_PW_P0, NRF24_PAYLOAD_SIZE);
    nrf_write_register(REG_EN_RXADDR, (0x1U << 0U));
    nrf_write_register(REG_EN_AA, (0x1U << 0U));
    nrf_write_register(REG_SETUP_AW, NRF24_ADDR_SIZE - 2U);

    static const uint8_t address[NRF24_ADDR_SIZE] = {0xAA, 0xBB, 0xCC};
    nrf_write_register_multi(REG_TX_ADDR, address, NRF24_ADDR_SIZE);
    nrf_write_register_multi(REG_RX_ADDR_P0, address, NRF24_ADDR_SIZE);

    nrf_command(CMD_FLUSH_TX);
    nrf_command(CMD_FLUSH_RX);
    nrf_write_register(REG_STATUS, (1U << STATUS_RX_DR) | (1U << STATUS_TX_DS) | (1U << STATUS_MAX_RT));

    delay_10us(200U);

    return nrf_set_rx_mode();
}

uint8_t nrf_receive_data(uint8_t *data, uint16_t length)
{
    if ((data == NULL) || (length != NRF24_PAYLOAD_SIZE))
    {
        return 0U;
    }

    uint8_t txbuf[NRF24_PAYLOAD_SIZE + 1U];
    uint8_t rxbuf[NRF24_PAYLOAD_SIZE + 1U];

    txbuf[0] = CMD_R_RX_PAYLOAD;

    for (uint16_t i = 0U; i < NRF24_PAYLOAD_SIZE; i++)
    {
        txbuf[i + 1U] = CMD_NOP;
    }

    if (nrf_spi_transfer_dma(txbuf, rxbuf, NRF24_PAYLOAD_SIZE + 1U) == 0U)
    {
        return 0U;
    }

    for (uint16_t i = 0U; i < NRF24_PAYLOAD_SIZE; i++)
    {
        data[i] = rxbuf[i + 1U];
    }

    return 1U;
}

uint8_t nrf_send_data(const uint8_t *data, uint16_t length)
{
    if(nrf_set_tx_mode() == 0U)
    {
        return 0U;
    }

    if ((data == NULL) || (length != NRF24_PAYLOAD_SIZE))
    {
        return 0U;
    }

    uint8_t txbuf[NRF24_PAYLOAD_SIZE + 1U];
    uint8_t rxbuf[NRF24_PAYLOAD_SIZE + 1U];

    txbuf[0] = CMD_W_TX_PAYLOAD;

    for (uint16_t i = 0U; i < length; i++)
    {
        txbuf[i + 1U] = data[i];
    }

    nrf_ce_low();

    if (nrf_spi_transfer_dma(txbuf, rxbuf, length + 1U) == 0U)
    {
        return 0U;
    }

    nrf_ce_high();
    delay_10us(2U);
    nrf_ce_low();

    return 1U;
}

void nrf_handle_tx_status(void)
{
    uint8_t status;
    nrf_read_register(REG_STATUS, &status);

    if ((status & (1U << STATUS_TX_DS)) != 0U)
    {
        nrf_write_register(REG_STATUS, (1U << STATUS_TX_DS));
    }

    if ((status & (1U << STATUS_MAX_RT)) != 0U)
    {
        nrf_write_register(REG_STATUS, (1U << STATUS_MAX_RT));
        nrf_command(CMD_FLUSH_TX);
    }
}


uint8_t nrf_read_data(uint8_t *data, uint16_t length)
{
    uint8_t status;

    if (nrf_read_register(REG_STATUS, &status) == 0U)
    {
        return 0U;
    }

    if ((status & (1U << STATUS_RX_DR)) == 0U)
    {
        return 0U;
    }

    if (nrf_receive_data(data, length) == 0U)
    {
        return 0U;
    }

    nrf_write_register(REG_STATUS, (1U << STATUS_RX_DR));

    return 1U;
}

void nrf_flush_rx(void)
{
    nrf_command(CMD_FLUSH_RX);
}

uint8_t nrf_process_irq(uint8_t *rx_data)
{
    uint8_t status;
    uint8_t event = NRF_EVENT_NONE;

    if (nrf_read_register(REG_STATUS, &status) == 0U)
    {
        return NRF_EVENT_ERROR;
    }

    if ((status & (1U << STATUS_RX_DR)) != 0U)
    {
        if (nrf_receive_data(rx_data, NRF24_PAYLOAD_SIZE) != 0U)
        {
            event |= NRF_EVENT_RX_DATA;
        }
        nrf_write_register(REG_STATUS, (1U << STATUS_RX_DR));
    }

    if ((status & (1U << STATUS_TX_DS)) != 0U)
    {
        nrf_write_register(REG_STATUS, (1U << STATUS_TX_DS));
        nrf_set_rx_mode();
        event |= NRF_EVENT_TX_OK;
    }

    if ((status & (1U << STATUS_MAX_RT)) != 0U)
    {
        nrf_write_register(REG_STATUS, (1U << STATUS_MAX_RT));
        nrf_command(CMD_FLUSH_TX);
        nrf_set_rx_mode();
        event |= NRF_EVENT_TX_FAILED;
    }

    return event;
}

void EXTI7_IRQHandler(void)
{
    if ((EXTI->FPR1 & (1U << NRF_IRQ_PIN)) != 0U)
    {
        EXTI->FPR1 = (1U << NRF_IRQ_PIN);
        nrf_irq = 1U;
    }
}