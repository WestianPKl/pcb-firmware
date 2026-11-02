#include "adc.h"

volatile uint16_t adc_buffer[ADC_CHANNEL_COUNT];
volatile uint16_t adc_vin = 0;
volatile uint16_t adc_v12 = 0;
volatile uint16_t adc_v5 = 0;
volatile uint16_t adc_v33 = 0;
volatile uint8_t adc_data_valid = 0U;

static volatile uint32_t adc_scan_count = 0U;

void adc1_init(void)
{
    RCC->IOPENR |= RCC_IOPENR_GPIOAEN;
    (void)RCC->IOPENR;

    GPIOA->MODER &= ~((0x3U << (VIN_PIN * 2U)) | (0x3U << (V12_PIN * 2U)) | (0x3U << (V5_PIN * 2U)) | (0x3U << (V33_PIN * 2U)));
    GPIOA->MODER |= ((0x3U << (VIN_PIN * 2U)) | (0x3U << (V12_PIN * 2U)) | (0x3U << (V5_PIN * 2U)) | (0x3U << (V33_PIN * 2U)));
    GPIOA->PUPDR &= ~((0x3U << (VIN_PIN * 2U)) | (0x3U << (V12_PIN * 2U)) | (0x3U << (V5_PIN * 2U)) | (0x3U << (V33_PIN * 2U)));

    RCC->APBENR2 |= RCC_APBENR2_ADCEN;
    (void)RCC->APBENR2;

    ADC1->CR &= ~(ADC_CR_ADEN_Msk);

    ADC1->CR |= ADC_CR_ADVREGEN_Msk;
    ADC1->CR |= ADC_CR_ADCAL_Msk;
    while ((ADC1->CR & ADC_CR_ADCAL_Msk) != 0U){}

    ADC1->CHSELR = ADC_CHSELR_CHSEL0 | ADC_CHSELR_CHSEL1 | ADC_CHSELR_CHSEL2 | ADC_CHSELR_CHSEL3;

    ADC1->SMPR &= ~ADC_SMPR_SMP1_Msk;
    ADC1->SMPR |= (0x7U << ADC_SMPR_SMP1_Pos);

    ADC1->ISR = ADC_ISR_ADRDY_Msk;
    ADC1->CFGR1 |= ADC_CFGR1_CONT_Msk | ADC_CFGR1_DMAEN_Msk | ADC_CFGR1_DMACFG_Msk;
    ADC1->CR |= (0x1U << ADC_CR_ADEN_Pos);
    while ((ADC1->ISR & ADC_ISR_ADRDY_Msk) == 0U){}
}

void adc1_start(void)
{
    ADC1->CR |= ADC_CR_ADSTART_Msk;
}

void adc1_dma_init(void)
{
    DMA1_Channel1->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos));
    while ((DMA1_Channel1->CCR & (0x1U << DMA_CCR_EN_Pos)) != 0U) {}

    DMA1_Channel1->CPAR = (uint32_t)&ADC1->DR;
    DMA1_Channel1->CMAR = (uint32_t)adc_buffer;
    DMA1_Channel1->CNDTR = ADC_CHANNEL_COUNT;

    DMA1_Channel1->CCR = 0;
    DMA1_Channel1->CCR |=
        (0x1U << DMA_CCR_PL_Pos)    |
        (0x1U << DMA_CCR_MSIZE_Pos) |
        (0x1U << DMA_CCR_PSIZE_Pos) |
        DMA_CCR_CIRC_Msk            |
        DMA_CCR_MINC_Msk            |
        DMA_CCR_TCIE_Msk;

    DMA1->IFCR = 0xFU << (4U * (1U - 1U));

    NVIC_ClearPendingIRQ(DMA1_Channel1_IRQn);
    NVIC_SetPriority(DMA1_Channel1_IRQn, 2U);
    NVIC_EnableIRQ(DMA1_Channel1_IRQn);

    DMA1_Channel1->CCR |= (0x1U << DMA_CCR_EN_Pos);
}

void adc_update(void)
{
    static uint32_t previous_scan_count = 0U;
    uint32_t current_scan_count = adc_scan_count;
    adc_data_valid = 0U;

    if (current_scan_count == previous_scan_count)
    {
        return;
    }

    previous_scan_count = current_scan_count;
    adc_vin = ((uint32_t)adc_buffer[0] * VDDA_MV * 23U) / (4095U * 3U);
    adc_v12 = ((uint32_t)adc_buffer[1] * VDDA_MV * 23U) / (4095U * 3U);
    adc_v5 = ((uint32_t)adc_buffer[2] * VDDA_MV * 95U) / (4095U * 27U);
    adc_v33 = ((uint32_t)adc_buffer[3] * VDDA_MV * 20U) / (4095U * 9U);

    adc_data_valid = 1U;
}

void DMA1_Channel1_IRQHandler(void)
{
    if ((DMA1->ISR & DMA_ISR_TCIF1) != 0U)
    {
        DMA1->IFCR = DMA_IFCR_CTCIF1;
        adc_scan_count++;
    }
}