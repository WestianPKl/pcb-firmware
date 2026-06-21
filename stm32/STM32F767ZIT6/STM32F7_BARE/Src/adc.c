#include "adc.h"
#include "support.h"

void adc_init(void)
{
    set_ahb1_periph_clock(RCC_AHB1ENR_GPIOAEN); // enable GPIOA clock
    set_pin_mode(GPIOA, 4U, 0x0U); // set PA4 to analog mode

    set_apb2_periph_clock(RCC_APB2ENR_ADC1EN); // enable ADC1 clock

    ADC1->CR1 = 0U; // reset CR1 register
    ADC1->CR2 = 0U; // reset CR2 register
    ADC1->SQR1 = 0U; // one conversion in the regular sequence

    ADC1->CR2 |= ADC_CR2_CONT; // enable continuous conversion mode

    ADC1->SQR3 = ADC1_CH4; // select channel 4 (PA4) for conversion

    ADC1->CR2 |= ADC_CR2_ADON; // enable ADC
    ADC1->CR2 |= ADC_CR2_SWSTART; // start conversion
}

uint32_t adc_read(void)
{
    while (!(ADC1->SR & ADC_SR_EOC)){}; // wait for conversion to complete
    return (uint32_t)(ADC1->DR & 0xFFF); // read 12-bit result
}