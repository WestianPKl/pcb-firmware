#include "timer.h"
#include "input.h"
#include "lwip/arch.h"

u32_t sys_now(void)
{
    return (u32_t)time_ms;
}

volatile uint32_t tim3_compare_count = 0U;
volatile uint8_t compare_1s_flag = 0U;

volatile uint16_t capture_previous = 0U;
volatile uint16_t capture_current = 0U;
volatile uint16_t capture_delta = 0U;
volatile uint8_t capture_first = 0U;
volatile uint8_t capture_done = 0U;
volatile uint16_t capture_high_ticks = 0U;
volatile uint8_t capture_high_valid = 0U;

static uint8_t tick_div = 0U;
volatile uint32_t time_10us = 0U;
volatile uint32_t time_ms = 0U;
volatile uint8_t clock_tick_flag_us = 0U;
volatile uint32_t tick_10ms = 0U;
volatile uint8_t clock_tick_flag = 0U;

volatile uint32_t lptim_seconds = 0U;
volatile uint8_t lptim_state = 0U;

void delay_10us(uint32_t ticks)
{
    uint32_t start = time_10us;
    while ((uint32_t)(time_10us - start) < ticks){}
}


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

void tim6_init(void)
{
    uint32_t timclk;
    uint32_t psc;
    uint32_t arr;

    RCC->APB1LENR |= RCC_APB1LENR_TIM6EN;
    (void)RCC->APB1LENR;

    TIM6->CR1 = 0U;
    TIM6->CR2 = 0U;

    timclk = tim_apb1_clock_hz();

    psc = (timclk / 10000000U);
    if (psc == 0U) {
        psc = 1U;
    }

    TIM6->PSC = psc - 1U;

    arr = 100U;
    TIM6->ARR = arr - 1U;
    TIM6->EGR = TIM_EGR_UG;
    TIM6->SR = 0;
    TIM6->DIER |= TIM_DIER_UIE;

    NVIC->IPR[49] = 0x50U;
    NVIC->ICPR[1] = (1UL << 17U);
    NVIC->ISER[1] = (1UL << 17U);

    TIM6->CR1 |= TIM_CR1_CEN;
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

uint32_t millis(void)
{
    return time_ms;
}

uint8_t lptim1_init_1s(void)
{
    uint32_t timeout;

    /*
     * LSE już mamy uruchomione przez RTC.
     */
    if ((RCC->BDCR & RCC_BDCR_LSERDY) == 0U)
    {
        return 0U;
    }

    /*
     * LPTIM1 kernel clock = LSE.
     *
     * LPTIM1SEL:
     * 011 = LSE
     */
    RCC->CCIPR2 &= ~RCC_CCIPR2_LPTIM1SEL_Msk;
    RCC->CCIPR2 |=
        RCC_CCIPR2_LPTIM1SEL_0 |
        RCC_CCIPR2_LPTIM1SEL_1;

    /*
     * Włącz zegar interfejsu LPTIM1 na APB3.
     */
    RCC->APB3ENR |= RCC_APB3ENR_LPTIM1EN;
    (void)RCC->APB3ENR;

    /*
     * Reset peryferium.
     */
    RCC->APB3RSTR |= RCC_APB3RSTR_LPTIM1RST;
    RCC->APB3RSTR &= ~RCC_APB3RSTR_LPTIM1RST;

    /*
     * CKSEL = 0 -> internal kernel clock.
     *
     * PRESC = 101 -> /32
     *
     * 32768 / 32 = 1024 Hz
     */
    LPTIM1->CFGR = 0U;

    LPTIM1->CFGR |=
        (5U << LPTIM_CFGR_PRESC_Pos);

    /*
     * W H5 zapisy ARR/DIER są synchronizowane.
     * Najpierw musimy ENABLE LPTIM.
     */
    LPTIM1->CR = LPTIM_CR_ENABLE;

    /*
     * ARR = 1023:
     *
     * licznik:
     * 0 ... 1023 = 1024 ticki
     *
     * 1024 / 1024 Hz = 1 s
     */
    LPTIM1->ICR = LPTIM_ICR_ARROKCF;

    LPTIM1->ARR = 1023U;

    timeout = 1000000U;

    while ((LPTIM1->ISR & LPTIM_ISR_ARROK) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    LPTIM1->ICR = LPTIM_ICR_ARROKCF;

    /*
     * Usuń ewentualny stary ARR match.
     */
    LPTIM1->ICR = LPTIM_ICR_ARRMCF;

    /*
     * Włącz interrupt ARR match.
     *
     * Zapis DIER też ma synchronizację.
     */
    LPTIM1->ICR = LPTIM_ICR_DIEROKCF;

    LPTIM1->DIER = LPTIM_DIER_ARRMIE;

    timeout = 1000000U;

    while ((LPTIM1->ISR & LPTIM_ISR_DIEROK) == 0U)
    {
        if (--timeout == 0U)
        {
            return 0U;
        }
    }

    LPTIM1->ICR = LPTIM_ICR_DIEROKCF;

    /*
     * NVIC.
     */
    NVIC_ClearPendingIRQ(LPTIM1_IRQn);
    NVIC_SetPriority(LPTIM1_IRQn, 5U);
    NVIC_EnableIRQ(LPTIM1_IRQn);

    LPTIM1->ICR =
    LPTIM_ICR_CC1CF |
    LPTIM_ICR_CC2CF |
    LPTIM_ICR_UECF |
    LPTIM_ICR_ARRMCF;
    
    /*
     * Start continuous.
     */
    LPTIM1->CR |= LPTIM_CR_CNTSTRT;

    return 1U;
}


void TIM6_IRQHandler(void)
{
    if (TIM6->SR & TIM_SR_UIF)
    {
        TIM6->SR = ~TIM_SR_UIF;

        time_10us++;
        static uint8_t ms_divider = 0U;
        ms_divider++;

        if (ms_divider >= 100U)
        {
            ms_divider = 0U;
            time_ms++;
        }

        if (++tick_div >= 10U)
        {
            tick_div = 0U;
            clock_tick_flag_us = 1U;
        }
    }
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

void LPTIM1_IRQHandler(void)
{
    if ((LPTIM1->ISR & LPTIM_ISR_ARRM) != 0U)
    {
        LPTIM1->ICR = LPTIM_ICR_ARRMCF;

        lptim_seconds++;
    }
}