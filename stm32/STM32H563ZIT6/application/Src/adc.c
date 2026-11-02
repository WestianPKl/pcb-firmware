#include "adc.h"
#include "timer.h"
#include "support.h"

volatile uint16_t adc_buffer[100];
volatile uint8_t adc_dma_ready = 0U;
volatile uint8_t adc_dma_half_ready = 0U;
static volatile uint8_t adc_dma_error = 0U;

static uint32_t adc_dma_node_dar __attribute__((aligned(4)));

void adc1_init(void)
{
    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->MODER &= ~(0x3U << (ADC1_PIN * 2U));
    GPIOA->MODER |= (0x3U << (ADC1_PIN * 2U));
    GPIOA->PUPDR &= ~(0x3U << (ADC1_PIN * 2U));

    RCC->AHB2ENR |= RCC_AHB2ENR_ADCEN;
    (void)RCC->AHB2ENR;

    ADC1->CR &= ~(ADC_CR_ADEN_Msk);

    ADC1->OR |= ADC_OR_OP0_Msk;

    ADC12_COMMON->CCR &= ~ADC_CCR_CKMODE_Msk;
    ADC12_COMMON->CCR |= (0x3U << ADC_CCR_CKMODE_Pos);

    ADC1->CR &= ~ADC_CR_DEEPPWD_Msk;
    ADC1->CR |= ADC_CR_ADVREGEN_Msk;

    delay_10us(10U);
    
    ADC1->CFGR &= ~(ADC_CFGR_CONT_Msk);

    ADC1->CR &= ~ADC_CR_ADCALDIF_Msk;
    ADC1->CR |= ADC_CR_ADCAL_Msk;

    while ((ADC1->CR & ADC_CR_ADCAL_Msk) != 0U){}

    ADC1->SMPR1 &= ~(ADC_SMPR1_SMP0_Msk);
    ADC1->SMPR1 |= (0x4U << ADC_SMPR1_SMP0_Pos);

    ADC1->SQR1 &= ~(ADC_SQR1_L_Msk);
    ADC1->SQR1 &= ~(ADC_SQR1_SQ1_Msk);
    ADC1->SQR1 |= (ADC1_CH0 << ADC_SQR1_SQ1_Pos);

    ADC1->ISR = ADC_ISR_ADRDY_Msk;
    ADC1->CR |= (0x1U << ADC_CR_ADEN_Pos);
    
    while ((ADC1->ISR & ADC_ISR_ADRDY_Msk) == 0U){}
}

void adc1_trigg_init(void)
{
    uint32_t timclk;
    uint32_t psc;

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    TIM1->CR1 = 0U;
    TIM1->CR2 = 0U;

    TIM1->CR2 |= TIM_CR2_MMS_1;

    timclk = tim_apb2_clock_hz();

    psc = timclk / 100000U;

    TIM1->PSC = psc - 1U;
    TIM1->ARR = 100U - 1U;

    TIM1->EGR = TIM_EGR_UG;
    TIM1->SR = 0U;

    ADC1->CFGR &= ~ADC_CFGR_EXTEN_Msk;
    ADC1->CFGR |= (0x1U << ADC_CFGR_EXTEN_Pos);

    ADC1->CFGR &= ~ADC_CFGR_EXTSEL_Msk;
    ADC1->CFGR |= (0x9U << ADC_CFGR_EXTSEL_Pos);

    ADC1->ISR = ADC_ISR_EOC_Msk | ADC_ISR_EOS_Msk | ADC_ISR_OVR_Msk;

    ADC1->CFGR |= ADC_CFGR_OVRMOD_Msk;

    // ADC1->CR |= ADC_CR_ADSTART_Msk;
    // TIM1->CR1 |= TIM_CR1_CEN;
}

void adc1_dma_init(void)
{
    RCC->AHB1ENR |= RCC_AHB1ENR_GPDMA1EN;
    (void)RCC->AHB1ENR;

    GPDMA1_Channel6->CCR &= ~((0x1U << DMA_CCR_EN_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_DTEIE_Pos) | (0x1U << DMA_CCR_HTIE_Pos));

    GPDMA1_Channel6->CTR1 &= ~((0x3U << DMA_CTR1_SDW_LOG2_Pos) | (0x3U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_SINC_Pos) | (0x3F << DMA_CTR1_SBL_1_Pos) | (0x3F << DMA_CTR1_DBL_1_Pos));
    GPDMA1_Channel6->CTR1 |= (0x1U << DMA_CTR1_SDW_LOG2_Pos) | (0x1U << DMA_CTR1_DDW_LOG2_Pos) | (0x1U << DMA_CTR1_DINC_Pos);

    GPDMA1_Channel6->CTR2 &= ~((0xFFU << DMA_CTR2_REQSEL_Pos) | (0x1U << DMA_CTR2_SWREQ_Pos) | (0x1U << DMA_CTR2_DREQ_Pos) | (0x1U << DMA_CTR2_PFREQ_Pos));
    GPDMA1_Channel6->CTR2 |= (0x0U << DMA_CTR2_REQSEL_Pos);

    adc_dma_node_dar = (uint32_t)adc_buffer;
    uint32_t node_addr = (uint32_t)&adc_dma_node_dar;

    GPDMA1_Channel6->CLBAR = node_addr & 0xFFFF0000UL;
    GPDMA1_Channel6->CLLR = (node_addr & 0x0000FFFCUL) | (0x1UL << DMA_CLLR_UDA_Pos);

    GPDMA1_Channel6->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos) | (0x1U << DMA_CFCR_HTF_Pos);
    GPDMA1_Channel6->CCR |= (0x1U << DMA_CCR_DTEIE_Pos) | (0x1U << DMA_CCR_TCIE_Pos) | (0x1U << DMA_CCR_HTIE_Pos);

    NVIC->IPR[33] = 0x50U;
    NVIC->ICPR[1] = (1U << 1U);
    NVIC->ISER[1] = (1U << 1U);

    ADC1->CFGR |= (0x1U << ADC_CFGR_DMACFG_Pos);
    ADC1->CFGR |=  (0x1U << ADC_CFGR_DMAEN_Pos);
}

uint8_t adc1_dma_start(void)
{
    if ((ADC1->CR & ADC_CR_ADSTART_Msk) != 0U)
    {
        return 0U;
    }

    GPDMA1_Channel6->CCR &= ~(0x1U << DMA_CCR_EN_Pos);
    GPDMA1_Channel6->CFCR = (0x1U << DMA_CFCR_TCF_Pos) | (0x1U << DMA_CFCR_DTEF_Pos) | (0x1U << DMA_CFCR_HTF_Pos);

    GPDMA1_Channel6->CSAR = (uint32_t)&ADC1->DR;
    GPDMA1_Channel6->CDAR = (uint32_t)adc_buffer;
    GPDMA1_Channel6->CBR1 = sizeof(adc_buffer);

    GPDMA1_Channel6->CCR |= (0x1U << DMA_CCR_EN_Pos);

    ADC1->CR |= ADC_CR_ADSTART_Msk;
    TIM1->CR1 |= TIM_CR1_CEN;

    return 1U;
}

uint8_t adc1_read(uint16_t *value)
{
    if (value == NULL)
    {
        return 0U;
    }

    if ((ADC1->ISR & ADC_ISR_ADRDY_Msk) == 0U)
    {
        return 0U;
    }

    ADC1->CR |= ADC_CR_ADSTART_Msk;

    while ((ADC1->ISR & ADC_ISR_EOC_Msk) == 0U){}

    *value = (uint16_t)ADC1->DR;

    return 1U;
}

uint8_t adc1_trigg_read(uint16_t *value)
{
    if (value == NULL)
    {
        return 0U;
    }

    if ((ADC1->ISR & ADC_ISR_EOC_Msk) == 0U)
    {
        return 0U;
    }

    *value = (uint16_t)ADC1->DR;

    return 1U;
}

void GPDMA1_CH6_IRQHandler(void)
{
    uint32_t status = GPDMA1_Channel6->CSR;

    if ((status & (1U << DMA_CSR_DTEF_Pos)) != 0U)
    {
        GPDMA1_Channel6->CFCR = (1U << DMA_CFCR_DTEF_Pos);
        adc_dma_error = 1U;
    }

    if ((status & (1U << DMA_CSR_HTF_Pos)) != 0U)
    {
        GPDMA1_Channel6->CFCR = (1U << DMA_CFCR_HTF_Pos);
        adc_dma_half_ready = 1U;
    }

    if ((status & (1U << DMA_CSR_TCF_Pos)) != 0U)
    {
        GPDMA1_Channel6->CFCR = (1U << DMA_CFCR_TCF_Pos);
        adc_dma_ready = 1U;
    }
}