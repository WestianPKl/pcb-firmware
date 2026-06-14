#include "timer.h"

// TIM1 CH1: PA8

// TIM2 CH3: PB10

// TIM3 CH1: PC6
// TIM3 CH2: PC7
// TIM3 CH3: PC8

// TIM4 CH3: PB8
// TIM4 CH4: PB9

// TIM8 CH4: PC9

// TIM13 - TIMER FOR CYCLIC LCD REFRESH AND SHT40 MEASUREMENT 1s, NOT USED FOR PWM
// TIM14 - TIMER FOR CYCLIC SHT40 MEASUREMENT - 10min, NOT USED FOR PWM

#define TIM1_CH1_PIN 8U

#define TIM2_CH3_PIN 10U

#define TIM3_CH1_PIN 6U
#define TIM3_CH2_PIN 7U
#define TIM3_CH3_PIN 8U

#define TIM4_CH3_PIN 8U
#define TIM4_CH4_PIN 9U

#define TIM8_CH4_PIN 9U

static uint32_t tim_apb1_clock_hz(void)
{
    uint32_t pclk1;
    uint32_t hclk;
    uint32_t cfgr;
    uint32_t hpre_bits;
    uint32_t ppre1_bits;
    uint32_t ahb_div;
    uint32_t apb1_div;

    static const uint16_t ahb_presc_table[16] = {
        1, 1, 1, 1, 1, 1, 1, 1,   // 0xxx
        2, 4, 8, 16, 64, 128, 256, 512
    };

    static const uint8_t apb_presc_table[8] = {
        1, 1, 1, 1, 2, 4, 8, 16
    };

    cfgr = RCC->CFGR;

    hpre_bits  = (cfgr >> RCC_CFGR_HPRE_Pos)  & 0x0F;
    ppre1_bits = (cfgr >> RCC_CFGR_PPRE1_Pos) & 0x07;

    ahb_div  = ahb_presc_table[hpre_bits];
    apb1_div = apb_presc_table[ppre1_bits];

    hclk  = SystemCoreClock / ahb_div;
    pclk1 = hclk / apb1_div;

    if (apb1_div == 1U) {
        return pclk1;
    }

    return pclk1 * 2U;
}

static uint32_t tim_apb2_clock_hz(void)
{
    uint32_t pclk2;
    uint32_t hclk;
    uint32_t cfgr;
    uint32_t hpre_bits;
    uint32_t ppre2_bits;
    uint32_t ahb_div;
    uint32_t apb2_div;

    static const uint16_t ahb_presc_table[16] = {
        1, 1, 1, 1, 1, 1, 1, 1,
        2, 4, 8, 16, 64, 128, 256, 512
    };

    static const uint8_t apb_presc_table[8] = {
        1, 1, 1, 1, 2, 4, 8, 16
    };

    cfgr = RCC->CFGR;

    hpre_bits  = (cfgr >> RCC_CFGR_HPRE_Pos)  & 0x0F;
    ppre2_bits = (cfgr >> RCC_CFGR_PPRE2_Pos) & 0x07;

    ahb_div  = ahb_presc_table[hpre_bits];
    apb2_div = apb_presc_table[ppre2_bits];

    hclk  = SystemCoreClock / ahb_div;
    pclk2 = hclk / apb2_div;

    if (apb2_div == 1U) {
        return pclk2;
    }

    return pclk2 * 2U;
}

void timer1_init(uint32_t prescaler, uint32_t period){
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    TIM1->CR1 = 0;
    TIM1->CR2 = 0;

    TIM1->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM1->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM1->CR2 &= ~TIM_CR2_MMS_Msk;
    TIM1->CR2 |= (0x2U << TIM_CR2_MMS_Pos);

    TIM1->EGR = TIM_EGR_UG;
    TIM1->SR = 0;
    TIM1->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM1_CC_IRQn);
}

void timer2_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    TIM2->CR1 = 0;
    TIM2->CR2 = 0;

    TIM2->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM2->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM2->CR2 &= ~TIM_CR2_MMS_Msk;
    TIM2->CR2 |= (0x2U << TIM_CR2_MMS_Pos);

    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM2_IRQn);
}

void timer3_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    TIM3->CR1 = 0;
    TIM3->CR2 = 0;

    TIM3->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM3->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM3->CR2 &= ~TIM_CR2_MMS_Msk;
    TIM3->CR2 |= (0x2U << TIM_CR2_MMS_Pos);

    TIM3->EGR = TIM_EGR_UG;
    TIM3->SR = 0;
    TIM3->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM3_IRQn);
}

void timer8_init(uint32_t prescaler, uint32_t period){
    RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
    (void)RCC->APB2ENR;

    TIM8->CR1 = 0;
    TIM8->CR2 = 0;

    TIM8->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM8->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM8->CR2 &= ~TIM_CR2_MMS_Msk;
    TIM8->CR2 |= (0x2U << TIM_CR2_MMS_Pos);

    TIM8->EGR = TIM_EGR_UG;
    TIM8->SR = 0;
    TIM8->CR1 |= TIM_CR1_CEN;
}

void timer4_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;

    TIM4->CR1 = 0;
    TIM4->CR2 = 0;

    TIM4->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM4->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM4->CR2 &= ~TIM_CR2_MMS_Msk;
    TIM4->CR2 |= (0x2U << TIM_CR2_MMS_Pos);

    TIM4->EGR = TIM_EGR_UG;
    TIM4->SR = 0;
    TIM4->CR1 |= TIM_CR1_CEN;

    NVIC_EnableIRQ(TIM4_IRQn);
}

void timer13_init_10ms(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->APB1ENR |= RCC_APB1ENR_TIM13EN;
    (void)RCC->APB1ENR;

    TIM13->CR1 = 0;
    TIM13->CR2 = 0;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 10000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM13->PSC = psc - 1U;

    arr = 100U;
    TIM13->ARR = arr - 1U;

    TIM13->EGR = TIM_EGR_UG;
    TIM13->SR = 0;
    TIM13->DIER |= TIM_DIER_UIE;

    NVIC_EnableIRQ(TIM8_UP_TIM13_IRQn);
    TIM13->CR1 |= TIM_CR1_CEN;
}

void timer1_pwm_ch1_init(uint32_t prescaler, uint32_t period)
{
    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOAEN;
    (void)RCC->AHB1ENR;

    GPIOA->MODER &= ~(3U << (TIM1_CH1_PIN * 2U));
    GPIOA->MODER |=  (2U << (TIM1_CH1_PIN * 2U));

    GPIOA->OTYPER &= ~(1U << TIM1_CH1_PIN);
    GPIOA->OSPEEDR |= (2U << (TIM1_CH1_PIN * 2U));
    GPIOA->PUPDR &= ~(3U << (TIM1_CH1_PIN * 2U));

    GPIOA->AFR[TIM1_CH1_PIN / 8U] &= ~(0xFU << ((TIM1_CH1_PIN % 8U) * 4U));
    GPIOA->AFR[TIM1_CH1_PIN / 8U] |=  (0x1U << ((TIM1_CH1_PIN % 8U) * 4U));

    TIM1->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE);
    TIM1->CCMR1 |=  (0x6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;

    TIM1->CCER &= ~(TIM_CCER_CC1P | TIM_CCER_CC1NP);
    TIM1->CCER |= TIM_CCER_CC1E;

    TIM1->PSC = (prescaler > 0U) ? (prescaler - 1U) : 0U;
    TIM1->ARR = (period > 0U) ? (period - 1U) : 0U;

    TIM1->CCR1 = 0U;
    TIM1->CNT = 0U;

    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1 |= TIM_CR1_CEN;
}

void timer2_pwm_ch3_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM2EN;
    (void)RCC->APB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(3U << (TIM2_CH3_PIN * 2U));
    GPIOB->MODER |=  (2U << (TIM2_CH3_PIN * 2U));

    GPIOB->OTYPER &= ~(1U << TIM2_CH3_PIN);
    GPIOB->OSPEEDR |= (2U << (TIM2_CH3_PIN * 2U));
    GPIOB->PUPDR &= ~(3U << (TIM2_CH3_PIN * 2U));

    GPIOB->AFR[TIM2_CH3_PIN / 8U] &= ~(0xFU << ((TIM2_CH3_PIN % 8U) * 4U));
    GPIOB->AFR[TIM2_CH3_PIN / 8U] |=  (0x1U << ((TIM2_CH3_PIN % 8U) * 4U));

    TIM2->CCMR2 &= ~(TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3PE);

    TIM2->CCMR2 |= (0x6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;

    TIM2->CCER |= TIM_CCER_CC3E;

    TIM2->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM2->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM2->CCR3 = 0;
    TIM2->CNT = 0;

    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->EGR = TIM_EGR_UG;

    TIM2->CR1 |= TIM_CR1_CEN; 
}

void timer3_pwm_ch1_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    GPIOC->MODER &= ~(3U << (TIM3_CH1_PIN * 2U));
    GPIOC->MODER |=  (2U << (TIM3_CH1_PIN * 2U));

    GPIOC->OTYPER &= ~(1U << TIM3_CH1_PIN);
    GPIOC->OSPEEDR |= (2U << (TIM3_CH1_PIN * 2U));
    GPIOC->PUPDR &= ~(3U << (TIM3_CH1_PIN * 2U));

    GPIOC->AFR[TIM3_CH1_PIN / 8U] &= ~(0xFU << ((TIM3_CH1_PIN % 8U) * 4U));
    GPIOC->AFR[TIM3_CH1_PIN / 8U] |=  (0x2U << ((TIM3_CH1_PIN % 8U) * 4U));

    TIM3->CCMR1 &= ~(TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE);

    TIM3->CCMR1 |= (0x6U << TIM_CCMR1_OC1M_Pos) | TIM_CCMR1_OC1PE;

    TIM3->CCER |= TIM_CCER_CC1E;

    TIM3->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM3->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM3->CCR1 = 0;
    TIM3->CNT = 0;

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;

    TIM3->CR1 |= TIM_CR1_CEN; 
}

void timer3_pwm_ch2_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    GPIOC->MODER &= ~(3U << (TIM3_CH2_PIN * 2U));
    GPIOC->MODER |=  (2U << (TIM3_CH2_PIN * 2U));

    GPIOC->OTYPER &= ~(1U << TIM3_CH2_PIN);
    GPIOC->OSPEEDR |= (2U << (TIM3_CH2_PIN * 2U));
    GPIOC->PUPDR &= ~(3U << (TIM3_CH2_PIN * 2U));

    GPIOC->AFR[TIM3_CH2_PIN / 8U] &= ~(0xFU << ((TIM3_CH2_PIN % 8U) * 4U));
    GPIOC->AFR[TIM3_CH2_PIN / 8U] |=  (0x2U << ((TIM3_CH2_PIN % 8U) * 4U));

    TIM3->CCMR1 &= ~(TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2PE);

    TIM3->CCMR1 |= (0x6U << TIM_CCMR1_OC2M_Pos) | TIM_CCMR1_OC2PE;

    TIM3->CCER |= TIM_CCER_CC2E;

    TIM3->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM3->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM3->CCR2 = 0;
    TIM3->CNT = 0;

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;

    TIM3->CR1 |= TIM_CR1_CEN; 
}

void timer3_pwm_ch3_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM3EN;
    (void)RCC->APB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    GPIOC->MODER &= ~(3U << (TIM3_CH3_PIN * 2U));
    GPIOC->MODER |=  (2U << (TIM3_CH3_PIN * 2U));

    GPIOC->OTYPER &= ~(1U << TIM3_CH3_PIN);
    GPIOC->OSPEEDR |= (2U << (TIM3_CH3_PIN * 2U));
    GPIOC->PUPDR &= ~(3U << (TIM3_CH3_PIN * 2U));

    GPIOC->AFR[TIM3_CH3_PIN / 8U] &= ~(0xFU << ((TIM3_CH3_PIN % 8U) * 4U));
    GPIOC->AFR[TIM3_CH3_PIN / 8U] |=  (0x2U << ((TIM3_CH3_PIN % 8U) * 4U));

    TIM3->CCMR2 &= ~(TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3PE);

    TIM3->CCMR2 |= (0x6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;

    TIM3->CCER |= TIM_CCER_CC3E;

    TIM3->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM3->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM3->CCR3 = 0;
    TIM3->CNT = 0;

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;

    TIM3->CR1 |= TIM_CR1_CEN; 
}

void timer8_pwm_ch4_init(uint32_t prescaler, uint32_t period){
    RCC->APB2ENR |= RCC_APB2ENR_TIM8EN;
    (void)RCC->APB2ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOCEN;
    (void)RCC->AHB1ENR;

    GPIOC->MODER &= ~(3U << (TIM8_CH4_PIN * 2U));
    GPIOC->MODER |=  (2U << (TIM8_CH4_PIN * 2U));

    GPIOC->OTYPER &= ~(1U << TIM8_CH4_PIN);
    GPIOC->OSPEEDR |= (2U << (TIM8_CH4_PIN * 2U));
    GPIOC->PUPDR &= ~(3U << (TIM8_CH4_PIN * 2U));

    GPIOC->AFR[TIM8_CH4_PIN / 8U] &= ~(0xFU << ((TIM8_CH4_PIN % 8U) * 4U));
    GPIOC->AFR[TIM8_CH4_PIN / 8U] |=  (0x3U << ((TIM8_CH4_PIN % 8U) * 4U));

    TIM8->CCMR2 &= ~(TIM_CCMR2_OC4M_Msk | TIM_CCMR2_OC4PE);

    TIM8->CCMR2 |= (0x6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;

    TIM8->CCER |= TIM_CCER_CC4E;

    TIM8->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM8->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM8->CCR4 = 0;
    TIM8->CNT = 0;

    TIM8->CR1 |= TIM_CR1_ARPE;
    TIM8->EGR = TIM_EGR_UG;
    TIM8->BDTR |= TIM_BDTR_MOE;
    TIM8->CR1 |= TIM_CR1_CEN; 
}

void timer4_pwm_ch3_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(3U << (TIM4_CH3_PIN * 2U));
    GPIOB->MODER |=  (2U << (TIM4_CH3_PIN * 2U));

    GPIOB->OTYPER &= ~(1U << TIM4_CH3_PIN);
    GPIOB->OSPEEDR |= (2U << (TIM4_CH3_PIN * 2U));
    GPIOB->PUPDR &= ~(3U << (TIM4_CH3_PIN * 2U));

    GPIOB->AFR[TIM4_CH3_PIN / 8U] &= ~(0xFU << ((TIM4_CH3_PIN % 8U) * 4U));
    GPIOB->AFR[TIM4_CH3_PIN / 8U] |=  (0x2U << ((TIM4_CH3_PIN % 8U) * 4U));

    TIM4->CCMR2 &= ~(TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3PE);

    TIM4->CCMR2 |= (0x6U << TIM_CCMR2_OC3M_Pos) | TIM_CCMR2_OC3PE;

    TIM4->CCER |= TIM_CCER_CC3E;

    TIM4->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM4->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM4->CCR3 = 0;
    TIM4->CNT = 0;

    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR = TIM_EGR_UG;

    TIM4->CR1 |= TIM_CR1_CEN; 
}

void timer4_pwm_ch4_init(uint32_t prescaler, uint32_t period){
    RCC->APB1ENR |= RCC_APB1ENR_TIM4EN;
    (void)RCC->APB1ENR;

    RCC->AHB1ENR |= RCC_AHB1ENR_GPIOBEN;
    (void)RCC->AHB1ENR;

    GPIOB->MODER &= ~(3U << (TIM4_CH4_PIN * 2U));
    GPIOB->MODER |=  (2U << (TIM4_CH4_PIN * 2U));

    GPIOB->OTYPER &= ~(1U << TIM4_CH4_PIN);
    GPIOB->OSPEEDR |= (2U << (TIM4_CH4_PIN * 2U));
    GPIOB->PUPDR &= ~(3U << (TIM4_CH4_PIN * 2U));

    GPIOB->AFR[TIM4_CH4_PIN / 8U] &= ~(0xFU << ((TIM4_CH4_PIN % 8U) * 4U));
    GPIOB->AFR[TIM4_CH4_PIN / 8U] |=  (0x2U << ((TIM4_CH4_PIN % 8U) * 4U));

    TIM4->CCMR2 &= ~(TIM_CCMR2_OC4M_Msk | TIM_CCMR2_OC4PE);

    TIM4->CCMR2 |= (0x6U << TIM_CCMR2_OC4M_Pos) | TIM_CCMR2_OC4PE;

    TIM4->CCER |= TIM_CCER_CC4E;

    TIM4->PSC = (prescaler > 0) ? (prescaler - 1U) : 0U;
    TIM4->ARR = (period > 0) ? (period - 1U) : 0U;

    TIM4->CCR4 = 0;
    TIM4->CNT = 0;

    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR = TIM_EGR_UG;

    TIM4->CR1 |= TIM_CR1_CEN; 
}

void timer1_pwm_ch1_set_duty(uint8_t duty_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    arr = TIM1->ARR;
    ccr = ((uint32_t)duty_percent * (arr + 1U)) / 100U;

    if (ccr > arr) {
        ccr = arr;
    }

    TIM1->CCR1 = ccr;
}


void timer2_pwm_ch3_set_duty(uint8_t duty_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    arr = TIM2->ARR;
    ccr = ((uint32_t)duty_percent * (arr + 1U)) / 100U;

    if (ccr > arr) {
        ccr = arr;
    }

    TIM2->CCR3 = ccr;
}


void timer3_pwm_ch1_set_duty(int32_t duty){
    if (duty < 0) duty = 0;
    if (duty > (int32_t)TIM3->ARR) duty = (int32_t)TIM3->ARR;
    TIM3->CCR1 = (uint32_t)duty;
}

void timer3_pwm_ch2_set_duty(int32_t duty){
    if (duty < 0) duty = 0;
    if (duty > (int32_t)TIM3->ARR) duty = (int32_t)TIM3->ARR;
    TIM3->CCR2 = (uint32_t)duty;
}

void timer3_pwm_ch3_set_duty(int32_t duty){
    if (duty < 0) duty = 0;
    if (duty > (int32_t)TIM3->ARR) duty = (int32_t)TIM3->ARR;
    TIM3->CCR3 = (uint32_t)duty;
}

void timer8_pwm_ch4_set_duty(int32_t duty){
    if (duty < 0) duty = 0;
    if (duty > (int32_t)TIM8->ARR) duty = (int32_t)TIM8->ARR;
    TIM8->CCR4 = (uint32_t)duty;
}

void timer4_pwm_ch3_set_duty(uint8_t duty_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    arr = TIM4->ARR;
    ccr = ((uint32_t)duty_percent * (arr + 1U)) / 100U;

    if (ccr > arr) {
        ccr = arr;
    }

    TIM4->CCR3 = ccr;
}

void timer4_pwm_ch4_set_duty(uint8_t duty_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    arr = TIM4->ARR;
    ccr = ((uint32_t)duty_percent * (arr + 1U)) / 100U;

    if (ccr > arr) {
        ccr = arr;
    }

    TIM4->CCR4 = ccr;
}


// TIMER3 CH1-CH3 LED RGB
void timer3_pwm_set_color(uint8_t r, uint8_t g, uint8_t b, uint8_t brightness)
{
    uint32_t arr = TIM3->ARR;

    if (brightness > 100U) {
        brightness = 100U;
    }

    uint32_t r_scaled = ((uint32_t)r * brightness) / 100U;
    uint32_t g_scaled = ((uint32_t)g * brightness) / 100U;
    uint32_t b_scaled = ((uint32_t)b * brightness) / 100U;

    uint32_t rd = (r_scaled * arr) / 255U;
    uint32_t gd = (g_scaled * arr) / 255U;
    uint32_t bd = (b_scaled * arr) / 255U;

    // CH3 - RED, CH2 - GREEN, CH1 - BLUE
    timer3_pwm_ch3_set_duty((int32_t)rd);
    timer3_pwm_ch2_set_duty((int32_t)gd);
    timer3_pwm_ch1_set_duty((int32_t)bd);
}

void timer8_pwm_set_buzzer_freq(uint32_t freq, uint32_t volume)
{
    uint32_t timer_clock;
    uint32_t prescaler = 1U;
    uint32_t period;

    if (volume > 100U) {
        volume = 100U;
    }

    if (freq == 0U || volume == 0U) {
        timer8_pwm_ch4_set_duty(0);
        return;
    }

    timer_clock = tim_apb2_clock_hz();
    period = timer_clock / freq;

    while (period > 65535U && prescaler < 65536U) {
        prescaler++;
        period = timer_clock / (prescaler * freq);
    }

    if (period < 2U) {
        period = 2U;
    }

    timer8_pwm_ch4_init(prescaler, period);

    timer8_pwm_ch4_set_duty((uint8_t)volume);
}