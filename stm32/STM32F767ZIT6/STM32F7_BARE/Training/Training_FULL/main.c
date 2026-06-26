#include "stm32f7xx.h"
#include "systick.h"
#include <stdio.h>
#include "support.h"
#include "uart.h"
#include "inputs.h"
#include "outputs.h"
#include "timers.h"
#include "adc.h"
#include "i2c.h"
#include "spi.h"

volatile uint8_t btn_pressed = 0;
volatile uint32_t tick_10ms = 0;
volatile uint32_t tick_100ms = 0;
volatile uint8_t clock_tick_flag = 0;

const uint8_t msg[] = "Hello";

uint8_t* p_tx_buffer = (uint8_t*)msg;
uint8_t tx_size = sizeof(msg); // Exclude null terminator
volatile uint8_t tx_idx = 0;

uint8_t rx_buffer[sizeof(msg)]; // Buffer to hold received data
uint8_t rx_size = sizeof(msg); // Size of the expected received data
volatile uint8_t rx_idx = 0;

uint8_t spi_tx_buffer[] = "Hello from SPI";
uint8_t spi_tx_size = sizeof(spi_tx_buffer); // Exclude null terminator
volatile uint8_t spi_tx_idx = 0;

uint8_t spi_rx_buffer[sizeof(spi_tx_buffer)]; // Buffer to hold received SPI data
uint8_t spi_rx_size = sizeof(spi_tx_buffer); // Size of the expected received SPI data
volatile uint8_t spi_rx_idx = 0;

static void handle_i2c1_master(void)
{
    I2C1->CR2 |= (I2C2_SLAVE_ADDRESS << 1U); // Set slave address (7-bit addr in SADD[7:1])
    I2C1->CR2 &= ~I2C_CR2_ADD10;

    I2C1->CR2 |= (tx_size << I2C_CR2_NBYTES_Pos); // Set number of bytes to send
    I2C1->CR2 |= I2C_CR2_AUTOEND; // Enable automatic end mode
    I2C1->CR2 |= I2C_CR2_START; // Generate start

    while (!(I2C1->ISR & I2C_ISR_STOPF)) {
        if (I2C1->ISR & I2C_ISR_TXIS) {
            if (tx_idx < tx_size) {
                I2C1->TXDR = p_tx_buffer[tx_idx++]; // Send next byte
            }
        }
    }

    I2C1->ICR |= I2C_ICR_STOPCF; // Clear STOP flag
}


int main(void)
{
    setvbuf(stdout, NULL, _IONBF, 0); // disable buffering for stdout
    init_leds();
    init_btn();
    systick_init();
    init_timer_TIM2_CH1();
    uart3_rxtx_init();
    adc_init_PA4_continuous();

    i2c2_slave_config();
    i2c1_master_config();
    handle_i2c1_master();

    spi1_config();
    spi3_config();
    spi_activate();
    // init_timer_TIM1_1hz();
    // systick_1hz_interrupt_init();
    // adc_init_PA4_interrupt();

	for(;;) {

        if (btn_pressed) {
            btn1_handler();
            btn_pressed = 0;
        }

        // while(!(TIM1->SR & TIM_SR_UIF)){}; // wait for TIM1 update event
        // TIM1->SR &= ~TIM_SR_UIF; // clear TIM1 update interrupt flag
        // printf("TIM1 Update Event\n");

        if (clock_tick_flag) {
            clock_tick_flag = 0;
            GPIOB->ODR ^= LED_2; // toggle LED2
            printf("ADC Value: %lu\n", adc_read_PA4()); // print ADC value
        }
    };
}

void EXTI15_10_IRQHandler(void)
{
    if (EXTI->PR & BTN_1) {
        EXTI->PR |= BTN_1; // clear pending bit
        btn_pressed = 1;
    }
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF) {
        TIM2->SR &= ~TIM_SR_UIF; // clear update interrupt flag
        tick_10ms++;

        if (tick_10ms >= 100) {
            tick_10ms = 0;
            clock_tick_flag = 1; // set clock tick flag every 1 second (100 * 10ms)
        }

        if (tick_10ms % 10 == 0) { // every 100ms
            tick_100ms++;
        }

        if (tick_100ms >= 10) {
            tick_100ms = 0;
            // Perform actions every 1 second
        }
    }
}

void ADC_IRQHandler(void)
{
    if (ADC1->SR & ADC_SR_EOC) {
        uint32_t adc_value = adc_read_PA4(); // read ADC value
        printf("ADC Value: %lu\n", adc_value); // print ADC value
        ADC1->SR &= ~ADC_SR_EOC; // clear end-of-conversion flag
    }
}

void USART3_IRQHandler(void)
{
    if ((USART3->ISR & USART_ISR_RXNE) == USART_ISR_RXNE) {
        uint8_t received_data;
        uart_read(USART3,&received_data); // read received data
        printf("Received: %c\n", received_data);
    }

    if ((USART3->ISR & USART_ISR_TXE) == USART_ISR_TXE) {
        // Handle transmit data register empty interrupt if needed
        // USART3->ISR &= ~USART_ISR_TXE; // clear transmit data register empty flag
    }
}

void I2C2_EV_IRQHandler(void)
{
    if (I2C2->ISR & I2C_ISR_ADDR) {
        if (i2c2_get_address_matchode() == I2C2_SLAVE_ADDRESS) {
            I2C2->ICR |= I2C_ICR_ADDRCF; // Clear address match flag
            I2C2->CR1 |= I2C_CR1_RXIE; // Enable RX interrupt
            rx_idx = 0; // Reset receive index for new transmission
        }
    }

    if (I2C2->ISR & I2C_ISR_RXNE) {
        if (rx_idx < rx_size) {
            rx_buffer[rx_idx++] = (I2C2->RXDR & 0xFFU); // Read received data
        }
    }

    if (I2C2->ISR & I2C_ISR_STOPF) {
        I2C2->ICR |= I2C_ICR_STOPCF; // Clear STOP flag
        printf("I2C Received Data: ");
        for (uint8_t i = 0; i < rx_idx; i++) {
            printf("%c ", rx_buffer[i]);
        }
        printf("\n");
    }
}


void SPI1_IRQHandler(void)
{
    if (SPI1->SR & SPI_SR_TXE) {
        if (spi_tx_idx < spi_tx_size) {
            volatile uint8_t *spidr = ((volatile uint8_t *)& SPI1->DR); // Access SPI data register as byte pointer
            *spidr = spi_tx_buffer[spi_tx_idx++]; // Send next byte over
        } else {
            SPI1->CR2 &= ~SPI_CR2_TXEIE; // All bytes sent: stop TXE interrupts (TXE stays set otherwise -> IRQ storm)
        }
    }
}

void SPI3_IRQHandler(void)
{
    if (SPI3->SR & SPI_SR_RXNE) {
        if (spi_rx_idx < spi_rx_size) {
            spi_rx_buffer[spi_rx_idx++] = SPI3->DR; // Read received SPI data
            printf("SPI Data Received: %c\n", spi_rx_buffer[spi_rx_idx - 1]); // Print received data
        }
    }
}
