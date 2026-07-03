#include "adc.h"
#include "support.h"

uint16_t adc_data_buffer[ADC_BUFFER_SIZE];

void adc1_ch4_continuous(uint8_t continuous)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~(0x3U << (ADC1_CH4 * 2U));
    GPIOA->MODER |=  (0x3U << (ADC1_CH4 * 2U));
    GPIOA->PUPDR &= ~(0x3U << (ADC1_CH4 * 2U));

    RCC->APB2ENR |= RCC_APB2ENR_ADC1EN;
    (void)RCC->APB2ENR;

    ADC1->CR1 = 0U;
    ADC1->CR2 = 0U;

    ADC1->SQR1 = 0;
    ADC1->SQR2 = 0;
    ADC1->SQR3 = 0;

    ADC1->SQR1 |= (0U << ADC_SQR1_L_Pos);
    ADC1->SQR3 |= (ADC1_CH4 << ADC_SQR3_SQ1_Pos);
    
    ADC1->SMPR2 &= ~(0x7U << ADC_SMPR2_SMP4_Pos);
    ADC1->SMPR2 |=  (0x2U << ADC_SMPR2_SMP4_Pos);

    ADC->CCR &= ~(ADC_CCR_ADCPRE_Msk);
    ADC->CCR |= ADC_CCR_ADCPRE_0;

    ADC1->CR1 |= ADC_CR1_SCAN;
    ADC1->CR2 |= ADC_CR2_DMA | ADC_CR2_DDS;

    if (continuous) ADC1->CR2 |= ADC_CR2_CONT;
    else                 ADC1->CR2 &= ~ADC_CR2_CONT;

    ADC1->CR2 |= ADC_CR2_ADON;
}

void adc1_start_conversion(void)
{
    ADC1->CR2 |= ADC_CR2_SWSTART;
}


void adc1_stop_conversion(void)
{
    ADC1->CR2 &= ~ADC_CR2_CONT;
    DMA2_Stream0->CR &= ~DMA_SxCR_EN;
    uint32_t t = 100000U;
    while ((DMA2_Stream0->CR & DMA_SxCR_EN) && --t) {}
}

uint8_t adc1_is_conversion_complete(void)
{
    if (!(DMA2_Stream0->CR & DMA_SxCR_CIRC)) {
        return (DMA2->LISR & DMA_LISR_TCIF0) ? 1U : 0U;
    }
    return 1U;
}

void adc1_clear_complete_flag(void)
{
    DMA2->LIFCR = DMA_LIFCR_CTCIF0;
}