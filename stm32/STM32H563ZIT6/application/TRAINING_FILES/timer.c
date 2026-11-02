#include "timer.h"
#include "input.h"

volatile uint32_t tim3_compare_count = 0;
volatile uint8_t compare_1s_flag;

volatile uint16_t capture_previous;
volatile uint16_t capture_current;
volatile uint16_t capture_delta;
volatile uint8_t capture_first = 0;
volatile uint8_t capture_done = 0;
volatile uint16_t capture_high_ticks;
volatile uint8_t capture_high_valid;

volatile uint32_t tick_10ms = 0;
volatile uint8_t clock_tick_flag = 0;


void tim4_pwm_input_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    GPIOB->MODER &= ~(0x3U << (TIM4_CH1_PIN * 2U));
    GPIOB->MODER |= (0x2U << (TIM4_CH1_PIN * 2U));
    GPIOB->OTYPER &= ~(0x1U << TIM4_CH1_PIN);
    GPIOB->OSPEEDR &= ~(0x3U << (TIM4_CH1_PIN * 2U));
    GPIOB->PUPDR &= ~(0x3U << (TIM4_CH1_PIN * 2U));

    GPIOB->AFR[0] &= ~(0xFU << (TIM4_CH1_PIN * 4U));
    GPIOB->AFR[0] |= (0x2U << (TIM4_CH1_PIN * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_TIM4EN;
    (void)RCC->APB1LENR;

    TIM4->CR1 = 0U;
    TIM4->CR2 = 0U;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 1000000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM4->PSC = psc - 1U;
    arr = 65536UL;
    TIM4->ARR = arr - 1U;

    TIM4->CCER &= ~(TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk |  TIM_CCER_CC1NP_Msk);
    TIM4->CCER &= ~(TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk |  TIM_CCER_CC2NP_Msk);

    TIM4->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_IC1PSC_Msk | TIM_CCMR1_IC1F_Msk);
    TIM4->CCMR1 &= ~(TIM_CCMR1_CC2S_Msk | TIM_CCMR1_IC2PSC_Msk | TIM_CCMR1_IC2F_Msk);
    TIM4->CCMR1 |= (0x1U << TIM_CCMR1_CC1S_Pos) | (0x2U << TIM_CCMR1_CC2S_Pos);

    TIM4->SMCR &= ~(TIM_SMCR_TS_Msk);
    TIM4->SMCR |= (0x5U << TIM_SMCR_TS_Pos);

    TIM4->SMCR &= ~(TIM_SMCR_SMS_Msk);
    TIM4->SMCR |= (0x4U << TIM_SMCR_SMS_Pos);

    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CCER |= TIM_CCER_CC2P | TIM_CCER_CC2E;

    TIM4->EGR = TIM_EGR_UG;
    TIM4-> SR = 0U;
    TIM4->DIER |= TIM_DIER_CC1IE | TIM_DIER_CC2IE;

    NVIC->IPR[47] = 0x50U;
    NVIC->ICPR[1] = (1UL << 15U);
    NVIC->ISER[1] = (1UL << 15U);

    TIM4->CR1 |= TIM_CR1_CEN;
}

uint8_t tim4_pwm_get_measurement(pwm_measurement_t *measurement)
{
    if (capture_done)
    {
        capture_done = 0U;

        if (capture_delta != 0U)
        {
            uint32_t period_ticks = (uint32_t)capture_delta + 1U;
            uint32_t high_ticks   = (uint32_t)capture_high_ticks + 1U;

            measurement->frequency_hz = 1000000U / period_ticks;
            measurement->duty_percent = (high_ticks * 100U) / period_ticks;
            return 1U;
        }
        else
        {
            measurement->frequency_hz = 0U;
            measurement->duty_percent = 0U;
            return 0U;
        }
    }
    return 0U;
}

void tim4_input_capture_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOBEN;
    (void)RCC->AHB2ENR;

    GPIOB->MODER &= ~(0x3U << (TIM4_CH1_PIN * 2U));
    GPIOB->MODER |= (0x2U << (TIM4_CH1_PIN * 2U));
    GPIOB->OTYPER &= ~(0x1U << TIM4_CH1_PIN);
    GPIOB->OSPEEDR &= ~(0x3U << (TIM4_CH1_PIN * 2U));
    GPIOB->PUPDR &= ~(0x3U << (TIM4_CH1_PIN * 2U));

    GPIOB->AFR[0] &= ~(0xFU << (TIM4_CH1_PIN * 4U));
    GPIOB->AFR[0] |= (0x2U << (TIM4_CH1_PIN * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_TIM4EN;
    (void)RCC->APB1LENR;

    TIM4->CR1 = 0U;
    TIM4->CR2 = 0U;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 1000000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM4->PSC = psc - 1U;
    arr = 65536UL;
    TIM4->ARR = arr - 1U;

    TIM4->CCER &= ~(TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk |  TIM_CCER_CC1NP_Msk);
    TIM4->CCER &= ~(TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk |  TIM_CCER_CC2NP_Msk);

    TIM4->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_IC1PSC_Msk | TIM_CCMR1_IC1F_Msk);
    TIM4->CCMR1 &= ~(TIM_CCMR1_CC2S_Msk | TIM_CCMR1_IC2PSC_Msk | TIM_CCMR1_IC2F_Msk);
    TIM4->CCMR1 |= (0x1U << TIM_CCMR1_CC1S_Pos) | (0x2U << TIM_CCMR1_CC2S_Pos);

    TIM4->CCER |= TIM_CCER_CC1E;
    TIM4->CCER |= TIM_CCER_CC2P | TIM_CCER_CC2E;

    TIM4->CR1 |= TIM_CR1_ARPE;
    TIM4->EGR = TIM_EGR_UG;
    TIM4-> SR = 0U;
    TIM4->DIER |= TIM_DIER_CC1IE | TIM_DIER_CC2IE;

    NVIC->IPR[47] = 0x50U;
    NVIC->ICPR[1] = (1UL << 15U);
    NVIC->ISER[1] = (1UL << 15U);

    TIM4->CR1 |= TIM_CR1_CEN;
}

void tim3_ch1_compare_match_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;

    GPIOC->MODER &= ~(0x3U << (PWM_PIN * 2U));
    GPIOC->MODER |= (0x2U << (PWM_PIN * 2U));
    GPIOC->OTYPER &= ~(0x1U << PWM_PIN);
    GPIOC->OSPEEDR &= ~(0x3U << (PWM_PIN * 2U));
    GPIOC->PUPDR &= ~(0x3U << (PWM_PIN * 2U));

    GPIOC->AFR[0] &= ~(0xFU << (PWM_PIN * 4U));
    GPIOC->AFR[0] |= (0x2U << (PWM_PIN * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_TIM3EN;
    (void)RCC->APB1LENR;

    TIM3->CR1 = 0U;
    TIM3->CR2 = 0U;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 1000000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM3->PSC = psc - 1U;
    arr = 250U;
    TIM3->ARR = arr - 1U;

    TIM3->CCER &= ~(TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk |  TIM_CCER_CC1NP_Msk);

    TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE_Msk );
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE | (0x3U << TIM_CCMR1_OC1M_Pos);

    TIM3->CCER |= TIM_CCER_CC1E;

    TIM3->CCR1 = 125U;

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;
    TIM3-> SR = 0U;
    TIM3->DIER |= TIM_DIER_CC1IE;

    NVIC->IPR[46] = 0x50U;
    NVIC->ICPR[1] = (1UL << 14U);
    NVIC->ISER[1] = (1UL << 14U);

    TIM3->CR1 |= TIM_CR1_CEN;
}

void tim3_ch1_pwm_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOCEN;
    (void)RCC->AHB2ENR;

    GPIOC->MODER &= ~(0x3U << (PWM_PIN * 2U));
    GPIOC->MODER |= (0x2U << (PWM_PIN * 2U));
    GPIOC->OTYPER &= ~(0x1U << PWM_PIN);
    GPIOC->OSPEEDR &= ~(0x3U << (PWM_PIN * 2U));
    GPIOC->PUPDR &= ~(0x3U << (PWM_PIN * 2U));

    GPIOC->AFR[0] &= ~(0xFU << (PWM_PIN * 4U));
    GPIOC->AFR[0] |= (0x2U << (PWM_PIN * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_TIM3EN;
    (void)RCC->APB1LENR;

    TIM3->CR1 = 0U;
    TIM3->CR2 = 0U;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 1000000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM3->PSC = psc - 1U;
    arr = 500U;
    TIM3->ARR = arr - 1U;

    TIM3->CCER &= ~(TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk |  TIM_CCER_CC1NP_Msk);

    TIM3->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE_Msk );
    TIM3->CCMR1 |= TIM_CCMR1_OC1PE | (0x6U << TIM_CCMR1_OC1M_Pos);

    TIM3->CCER |= TIM_CCER_CC1E;

    TIM3->CCR1 = 0U;

    TIM3->CR1 |= TIM_CR1_ARPE;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CR1 |= TIM_CR1_CEN;
}

void tim3_ch1_pwm_set_duty(uint8_t duty_percent)
{
    uint32_t arr;
    uint32_t ccr;

    if (duty_percent == 0U) {
        TIM3->CCER &= ~(TIM_CCER_CC1E_Msk);
        TIM3->CCR1 = 0U;
        return;
    }

    if (duty_percent > 100U) {
        duty_percent = 100U;
    }

    arr = TIM3->ARR;
    ccr = ((uint32_t)duty_percent * (arr + 1U)) / 100U;

    if (ccr > arr) {
        ccr = arr + 1;
    }

    TIM3->CCR1 = ccr;
    TIM3->CCER |= TIM_CCER_CC1E;
}

void tim3_ch1_pwm_set_frequency(uint32_t frequency_hz)
{
    uint32_t arr;

    if (frequency_hz == 0U) {
        TIM3->CCER &= ~(TIM_CCER_CC1E_Msk);
        TIM3->CCR1 = 0U;
        return;
    }

    arr = (1000000U / frequency_hz);
    if (arr == 0U) {
        arr = 1U;
    }

    TIM3->CCR1 = arr / 2U;
    TIM3->ARR = arr - 1U;
    TIM3->EGR = TIM_EGR_UG;
    TIM3->CCER |= TIM_CCER_CC1E;
}

void tim2_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;
    (void)RCC->APB1LENR;

    TIM2->CR1 = 0U;
    TIM2->CR2 = 0U;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 10000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM2->PSC = psc - 1U;

    arr = 100U;
    TIM2->ARR = arr - 1U;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0;
    TIM2->DIER |= TIM_DIER_UIE;

    NVIC->IPR[45] = 0x50U;
    NVIC->ICPR[1] = (1UL << 13U);
    NVIC->ISER[1] = (1UL << 13U);

    TIM2->CR1 |= TIM_CR1_CEN;
}

void TIM2_IRQHandler(void)
{
    if (TIM2->SR & TIM_SR_UIF)
    {
        TIM2->SR = ~TIM_SR_UIF;
        if (btn1_debounce_active)
        {
            if (btn1_debounce_ticks > 0U)
            {
                btn1_debounce_ticks--;

                if (btn1_debounce_ticks == 0U)
                {
                    btn1_debounce_active = 0U;
                    btn1_debounce_done = 1U;
                }
            }
        }
        if (++tick_10ms >= 100U) 
        {
            tick_10ms = 0;
            clock_tick_flag = 1;
        }
    }
}

void TIM3_IRQHandler(void)
{
    if (TIM3->SR & TIM_SR_CC1IF)
    {
        TIM3->SR = ~TIM_SR_CC1IF;
        if (++tim3_compare_count >= 4000UL)
        {
            tim3_compare_count = 0UL;
            compare_1s_flag = 1U;
        }
    }
}

void TIM4_IRQHandler(void)
{
    /* tim4_input_capture_init */
    // if (TIM4->SR & TIM_SR_CC1IF)
    // {
    //     uint16_t capture_value = (uint16_t)TIM4->CCR1;
    //     if (TIM4->SR & TIM_SR_CC1OF)
    //     {
    //         TIM4->SR = ~TIM_SR_CC1OF;
    //         capture_previous = capture_value;
    //         capture_first = 1U;
    //         capture_high_valid = 0U;
    //         capture_done = 0U;
    //     }
    //     else
    //     {
    //         if(capture_first == 0U)
    //         {
    //             capture_previous = capture_value;
    //             capture_first = 1U;
    //         }
    //         else
    //         {
    //             capture_current = capture_value;
    //             if (capture_current >= capture_previous)
    //             {
    //                 capture_delta = (uint16_t)(capture_current - capture_previous);
    //             }
    //             else
    //             {
    //                 capture_delta = (uint16_t)(65536U + capture_current - capture_previous);
    //             }
    //             capture_previous = capture_current;

    //             if (capture_high_valid)
    //             {
    //                 capture_done = 1U;
    //                 capture_high_valid = 0U;
    //             }
    //         }
    //     }
    // }

    // if (TIM4->SR & TIM_SR_CC2IF)
    // {
    //     uint16_t falling_timestamp = (uint16_t)TIM4->CCR2;
    //     if (TIM4->SR & TIM_SR_CC2OF)
    //     {
    //         TIM4->SR = ~TIM_SR_CC2OF;
    //         capture_high_ticks = 0U;
    //         capture_high_valid = 0U;
    //     }
    //     else 
    //     {
    //         capture_high_ticks = (uint16_t)(falling_timestamp - capture_previous);
    //         capture_high_valid = 1U;
    //     }
    // }

    /* tim4_pwm_input_init */
    if (TIM4->SR & TIM_SR_CC2IF)
    {
        uint16_t falling_timestamp = (uint16_t)TIM4->CCR2;
        if (TIM4->SR & TIM_SR_CC2OF)
        {
            TIM4->SR = ~TIM_SR_CC2OF;
            capture_high_ticks = 0U;
            capture_high_valid = 0U;
            capture_done = 0U;
        }
        else 
        {
            capture_high_ticks = falling_timestamp;
            capture_high_valid = 1U;
        }
    }

    if (TIM4->SR & TIM_SR_CC1IF)
    {
        uint16_t capture_value = (uint16_t)TIM4->CCR1;
        if (TIM4->SR & TIM_SR_CC1OF)
        {
            TIM4->SR = ~TIM_SR_CC1OF;
            capture_first = 1U;
            capture_high_valid = 0U;
            capture_done = 0U;
        }
        else
        {
            if(capture_first == 0U)
            {
                capture_first = 1U;
            }
            else
            {
                if (capture_high_valid)
                {
                    capture_delta = capture_value;
                    capture_done = 1U;
                    capture_high_valid = 0U;
                }
            }
        }
    }
}