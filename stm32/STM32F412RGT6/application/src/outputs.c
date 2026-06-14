#include "outputs.h"
#include "systick.h"

// PC4 -> ESP32 status out
// External: PC0..PC3, PB12
#define PC0_PIN 0U //Connector output
#define PC1_PIN 1U //Connector output
#define PC2_PIN 2U //Connector output
#define PC3_PIN 3U //Connector output 
#define PC4_PIN 4U //ESP32 status output

#define PB2_PIN 2U // ESP32 GPIO01 output
#define PB12_PIN 12U //Connector output
#define PB13_PIN 13U //ESP32 reset output
#define PB14_LED 14U 
#define PB15_LED 15U

void portc_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    uint32_t mask = (3U<<(PC0_PIN*2U))|(3U<<(PC1_PIN*2U))|(3U<<(PC2_PIN*2U))|(3U<<(PC3_PIN*2U))|(3U<<(PC4_PIN*2U));
    GPIOC->MODER &= ~mask;
    GPIOC->MODER |=  (1U<<(PC0_PIN*2U))|(1U<<(PC1_PIN*2U))|(1U<<(PC2_PIN*2U))|(1U<<(PC3_PIN*2U))|(1U<<(PC4_PIN*2U));

    GPIOC->OTYPER &= ~((1U<<PC0_PIN)|(1U<<PC1_PIN)|(1U<<PC2_PIN)|(1U<<PC3_PIN)|(1U<<PC4_PIN));
    GPIOC->OSPEEDR |= (2U<<(PC0_PIN*2U))|(2U<<(PC1_PIN*2U))|(2U<<(PC2_PIN*2U))|(2U<<(PC3_PIN*2U))|(2U<<(PC4_PIN*2U));
    GPIOC->PUPDR &= ~mask;

    pin_set_low('C', PC4_PIN);
}

void portb_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    uint32_t mask = (3U << (PB2_PIN*2U)) |(3U<<(PB12_PIN*2U)) |(3U<<(PB13_PIN*2U))|(3U<<(PB14_LED*2U))|(3U<<(PB15_LED*2U));
    GPIOB->MODER &= ~mask;
    GPIOB->MODER |=  (1U<<(PB2_PIN*2U))|(1U<<(PB12_PIN*2U))|(1U<<(PB13_PIN*2U))|(1U<<(PB14_LED*2U))|(1U<<(PB15_LED*2U));

    GPIOB->OTYPER &= ~((1U<<PB2_PIN)|(1U<<PB12_PIN)|(1U<<PB13_PIN)|(1U<<PB14_LED)|(1U<<PB15_LED));
    GPIOB->OSPEEDR |= (2U<<(PB2_PIN*2U))|(2U<<(PB12_PIN*2U))|(2U<<(PB13_PIN*2U))|(2U<<(PB14_LED*2U))|(2U<<(PB15_LED*2U));
    GPIOB->PUPDR &= ~mask;
}

void pin_set_high(uint8_t port, uint8_t pin)
{
    if (port == 'C') GPIOC->BSRR = (1U << pin);
    else if (port == 'B') GPIOB->BSRR = (1U << pin);
}

void pin_set_low(uint8_t port, uint8_t pin)
{
    if (port == 'C') GPIOC->BSRR = (1U << (pin + 16U));
    else if (port == 'B') GPIOB->BSRR = (1U << (pin + 16U));
}

void esp32_status_set(uint8_t status)
{
    if (status) pin_set_high('C', PC4_PIN);
    else        pin_set_low('C', PC4_PIN);
}

void esp32_reset(void){
    pin_set_high('B', PB13_PIN);
    systick_delay_ms(100);
    pin_set_low('B', PB13_PIN);
}