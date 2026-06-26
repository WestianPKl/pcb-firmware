#include "uart.h"
#include "support.h"

static uint16_t compute_uart_div(uint32_t pclk, uint32_t baudrate)
{
    return (pclk + (baudrate / 2U)) / baudrate; // round to nearest integer
}

static void uart_enable(USART_TypeDef *USARTx)
{
    SET_BIT(USARTx->CR1, USART_CR1_UE); // enable USART
}

static void uart_config(USART_TypeDef *USARTx, uint32_t DataWidth, uint32_t Parity, uint32_t StopBits)
{
    MODIFY_REG(USARTx->CR1, USART_CR1_M | USART_CR1_PCE | USART_CR1_PS, DataWidth | Parity);
    MODIFY_REG(USARTx->CR2, USART_CR2_STOP, StopBits);
}

static void uart_set_baudrate(USART_TypeDef *USARTx, uint32_t baudrate)
{
    uint32_t pclk = apb1_pclk_hz();
    USARTx->BRR = compute_uart_div(pclk, baudrate);
}

static void uart_set_direction(USART_TypeDef *USARTx, uint32_t direction)
{
    MODIFY_REG(USARTx->CR1, USART_CR1_TE | USART_CR1_RE, direction);
}

void uart_write(USART_TypeDef *USARTx, uint8_t data)
{
    while (!(USARTx->ISR & USART_ISR_TXE)){}; // wait until transmit data register is empty
    USARTx->TDR = data; // write data to transmit data register
}

void uart_read(USART_TypeDef *USARTx, uint8_t *data)
{
    while (!(USARTx->ISR & USART_ISR_RXNE)){}; // wait until receive data register is not empty
    *data = (uint8_t)(USARTx->RDR & 0xFF); // read data from receive data register
}

void uart3_rxtx_init(void)
{
    set_ahb1_periph_clock(RCC_AHB1ENR_GPIODEN);

    set_pin_mode(GPIOD, 8U, 0x2U); // set PD8 to alternate function mode
    GPIOD->AFR[1] &= ~(0xFU << (0U * 4U)); // clear AFRH8
    GPIOD->AFR[1] |=  (0x7U << (0U * 4U)); // set AFRH8 to AF7 (USART3_TX)

    set_pin_mode(GPIOD, 9U, 0x2U); // set PD9 to alternate function mode
    GPIOD->AFR[1] &= ~(0xFU << (1U * 4U)); // clear AFRH9
    GPIOD->AFR[1] |=  (0x7U << (1U * 4U)); // set AFRH9 to AF7 (USART3_RX)

    set_apb1_periph_clock(RCC_APB1ENR_USART3EN); // enable USART3 clock

    uart_config(USART3, 0U, 0U, 0U); // 8 data bits, no parity, 1 stop bit
    uart_set_direction(USART3, USART_CR1_TE | USART_CR1_RE); // enable transmitter and receiver
    uart_set_baudrate(USART3, 115200); // set baud rate to 115200

    uart_enable(USART3);

    // USART3->CR1 |= USART_CR1_TXEIE; // enable transmit data register empty interrupt
    USART3->CR1 |= USART_CR1_RXNEIE; // enable receive data register not empty interrupt

    NVIC_SetPriority(USART3_IRQn, 1); // set USART3 interrupt priority
    NVIC_EnableIRQ(USART3_IRQn); // enable USART3 interrupt
}


int __io_putchar(int ch)
{
    uart_write(USART3, (uint8_t)ch);
    return ch;
}

int _write(int fd, const char *buf, int len)
{
    (void)fd;
    for (int i = 0; i < len; i++) __io_putchar(buf[i]);
    return len;
}