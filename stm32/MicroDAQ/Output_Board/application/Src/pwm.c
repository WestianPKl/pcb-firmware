#include "pwm.h"
#include "output.h"
#include "support.h"
#include <stddef.h>
#include "version.h"

volatile pwm_status_t pwm_status = {0};
volatile uint8_t pwm_max = 0U;


static void tim1_pwm_init(void)
{
    uint32_t timclk;
    uint32_t psc;

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->BSRR = (1U << PWM0_PIN) |  (1U << PWM1_PIN) |  (1U << PWM2_PIN);
    GPIOA->MODER &= ~((3U << (PWM0_PIN * 2U)) |  (3U << (PWM1_PIN * 2U)) |  (3U << (PWM2_PIN * 2U)));
    GPIOA->MODER |= (1U << (PWM0_PIN * 2U)) | (1U << (PWM1_PIN * 2U)) | (1U << (PWM2_PIN * 2U));

    GPIOA->OTYPER &= ~((1U << PWM0_PIN) | (1U << PWM1_PIN) | (1U << PWM2_PIN));
    GPIOA->OSPEEDR &= ~((3U << (PWM0_PIN * 2U)) | (3U << (PWM1_PIN * 2U)) | (3U << (PWM2_PIN * 2U)));
    GPIOA->PUPDR &= ~((3U << (PWM0_PIN * 2U)) | (3U << (PWM1_PIN * 2U)) | (3U << (PWM2_PIN * 2U)));
    GPIOA->PUPDR |= ((1U << (PWM0_PIN * 2U)) | (1U << (PWM1_PIN * 2U)) | (1U << (PWM2_PIN * 2U)));

    GPIOA->AFR[1] &= ~((0xFU << ((PWM0_PIN & 7U) * 4U)) | (0xFU << ((PWM1_PIN & 7U) * 4U)) | (0xFU << ((PWM2_PIN & 7U) * 4U)));
    GPIOA->AFR[1] |= ((0x1U << ((PWM0_PIN & 7U) * 4U)) | (0x1U << ((PWM1_PIN & 7U) * 4U)) | (0x1U << ((PWM2_PIN & 7U) * 4U)));

    RCC->APB2ENR |= RCC_APB2ENR_TIM1EN;
    (void)RCC->APB2ENR;

    TIM1->CR1 = 0U;
    TIM1->CR2 = 0U;

    timclk = tim_apb2_clock_hz();
    uint32_t counter_hz = PWM_FREQUENCY_HZ * PWM_PERIOD_COUNTS;
    psc = timclk / counter_hz;

    if (psc == 0U)
    {
        psc = 1U;
    }

    TIM1->PSC = psc - 1U;
    TIM1->ARR = PWM_PERIOD_COUNTS - 1U;

    TIM1->CCER &= ~(TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk |  TIM_CCER_CC1NP_Msk);
    TIM1->CCER &= ~(TIM_CCER_CC2E_Msk | TIM_CCER_CC2P_Msk |  TIM_CCER_CC2NP_Msk);
    TIM1->CCER &= ~(TIM_CCER_CC3E_Msk | TIM_CCER_CC3P_Msk |  TIM_CCER_CC3NP_Msk);

    TIM1->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE_Msk );
    TIM1->CCMR1 |= TIM_CCMR1_OC1PE | (0x6U << TIM_CCMR1_OC1M_Pos);

    TIM1->CCMR1 &= ~(TIM_CCMR1_CC2S_Msk | TIM_CCMR1_OC2M_Msk | TIM_CCMR1_OC2PE_Msk );
    TIM1->CCMR1 |= TIM_CCMR1_OC2PE | (0x6U << TIM_CCMR1_OC2M_Pos);

    TIM1->CCMR2 &= ~(TIM_CCMR2_CC3S_Msk | TIM_CCMR2_OC3M_Msk | TIM_CCMR2_OC3PE_Msk );
    TIM1->CCMR2 |= TIM_CCMR2_OC3PE | (0x6U << TIM_CCMR2_OC3M_Pos);

    uint32_t period = TIM1->ARR + 1U;

    TIM1->CCR1 = period;
    TIM1->CCR2 = period;
    TIM1->CCR3 = period;

    TIM1->CR1 |= TIM_CR1_ARPE;
    TIM1->EGR = TIM_EGR_UG;
    TIM1->SR = 0U;
    TIM1->CCER |= TIM_CCER_CC1E | TIM_CCER_CC2E | TIM_CCER_CC3E;
    TIM1->BDTR |= TIM_BDTR_MOE;
    TIM1->CR1 |= TIM_CR1_CEN;

    GPIOA->MODER &= ~((3U << (PWM0_PIN * 2U)) | (3U << (PWM1_PIN * 2U)) | (3U << (PWM2_PIN * 2U)));
    GPIOA->MODER |= (2U << (PWM0_PIN * 2U)) | (2U << (PWM1_PIN * 2U)) | (2U << (PWM2_PIN * 2U));
}

static void tim2_pwm_init(void)
{
    uint32_t timclk;
    uint32_t psc;

    RCC->AHB2ENR |= RCC_AHB2ENR_GPIOAEN;
    (void)RCC->AHB2ENR;

    GPIOA->BSRR = (1U << PWM3_PIN);
    GPIOA->MODER &= ~(3U << (PWM3_PIN * 2U));
    GPIOA->MODER |=  (1U << (PWM3_PIN * 2U));

    GPIOA->OTYPER &= ~(1U << PWM3_PIN);
    GPIOA->OSPEEDR &= ~(3U << (PWM3_PIN * 2U));
    GPIOA->PUPDR &= ~(3U << (PWM3_PIN * 2U));
    GPIOA->PUPDR |= (1U << (PWM3_PIN * 2U));

    GPIOA->AFR[1] &= ~(0xFU << ((PWM3_PIN & 7U) * 4U));
    GPIOA->AFR[1] |=  (0x1U << ((PWM3_PIN & 7U) * 4U));

    RCC->APB1LENR |= RCC_APB1LENR_TIM2EN;
    (void)RCC->APB1LENR;

    TIM2->CR1 = 0U;
    TIM2->CR2 = 0U;

    timclk = tim_apb1_clock_hz();
    uint32_t counter_hz = PWM_FREQUENCY_HZ * PWM_PERIOD_COUNTS;
    psc = timclk / counter_hz;

    if (psc == 0U)
    {
        psc = 1U;
    }

    TIM2->PSC = psc - 1U;
    TIM2->ARR = PWM_PERIOD_COUNTS - 1U;

    TIM2->CCER &= ~(TIM_CCER_CC1E_Msk | TIM_CCER_CC1P_Msk |  TIM_CCER_CC1NP_Msk);

    TIM2->CCMR1 &= ~(TIM_CCMR1_CC1S_Msk | TIM_CCMR1_OC1M_Msk | TIM_CCMR1_OC1PE_Msk );
    TIM2->CCMR1 |= TIM_CCMR1_OC1PE | (0x6U << TIM_CCMR1_OC1M_Pos);

    TIM2->CCR1 = TIM2->ARR + 1U;
    TIM2->CR1 |= TIM_CR1_ARPE;
    TIM2->EGR = TIM_EGR_UG;
    TIM2->SR = 0U;

    TIM2->CCER |= TIM_CCER_CC1E;
    TIM2->CR1 |= TIM_CR1_CEN;

    GPIOA->MODER &= ~(3U << (PWM3_PIN * 2U));
    GPIOA->MODER |=  (2U << (PWM3_PIN * 2U));
}

static void tim1_ch1_set_duty(uint8_t duty)
{
    if (duty > 100U)
    {
        duty = 100U;
    }

    uint32_t period = TIM1->ARR + 1U;
    TIM1->CCR1 = ((uint32_t)(100U - duty) * period) / 100U;
}

static void tim1_ch2_set_duty(uint8_t duty)
{
    if (duty > 100U)
    {
        duty = 100U;
    }

    uint32_t period = TIM1->ARR + 1U;
    TIM1->CCR2 = ((uint32_t)(100U - duty) * period) / 100U;
}

static void tim1_ch3_set_duty(uint8_t duty)
{
    if (duty > 100U)
    {
        duty = 100U;
    }

    uint32_t period = TIM1->ARR + 1U;
    TIM1->CCR3 = ((uint32_t)(100U - duty) * period) / 100U;
}

static void tim2_ch1_set_duty(uint8_t duty)
{
    if (duty > 100U)
    {
        duty = 100U;
    }

    uint32_t period = TIM2->ARR + 1U;
    TIM2->CCR1 = ((uint32_t)(100U - duty) * period) / 100U;
}

void pwm_init(void)
{
    pwm_max = 0U;

    if (board_info_valid() != 0U)
    {
        const board_info_t *info = board_info_get();

        if ((info->capabilities.flags & CAP_PWM) != 0U)
        {
            pwm_max = info->capabilities.pwm_count;

            if (pwm_max > PWM_PHYSICAL_COUNT)
            {
                pwm_max = PWM_PHYSICAL_COUNT;
            }
        }
    }

    tim1_pwm_init();
    tim2_pwm_init();
    
    pwm_status.pwm0 = 0U;
    pwm_status.pwm1 = 0U;
    pwm_status.pwm2 = 0U;
    pwm_status.pwm3 = 0U;
}

uint8_t pwm_set_state(uint8_t pwm_number, uint8_t duty_cycle)
{
    if ((pwm_number >= pwm_max) || (duty_cycle > PWM_DUTY_MAX))
    {
        return 0U;
    }

    switch (pwm_number)
    {
        case 0U:
            tim1_ch1_set_duty(duty_cycle);
            pwm_status.pwm0 = duty_cycle;
            break;

        case 1U:
            tim1_ch2_set_duty(duty_cycle);
            pwm_status.pwm1 = duty_cycle;
            break;

        case 2U:
            tim1_ch3_set_duty(duty_cycle);
            pwm_status.pwm2 = duty_cycle;
            break;

        case 3U:
            tim2_ch1_set_duty(duty_cycle);
            pwm_status.pwm3 = duty_cycle;
            break;

        default:
            return 0U;
    }

    return 1U;
}

void pwm_get_status(pwm_status_t *status)
{
    if (status != NULL)
    {
        *status = pwm_status;
    }
}

uint8_t pwm_get_duty(uint8_t pwm_number)
{
    if (pwm_number >= pwm_max)
    {
        return 0U;
    }

    switch (pwm_number)
    {
        case 0U:
            return pwm_status.pwm0;

        case 1U:
            return pwm_status.pwm1;

        case 2U:
            return pwm_status.pwm2;

        case 3U:
            return pwm_status.pwm3;

        default:
            return 0U;
    }
}

uint16_t pwm_get_gpio_mask(void)
{
    uint16_t mask = 0U;

     if (TIM1->CCER & TIM_CCER_CC1E)  mask |= (1U << 0U);
     if (TIM1->CCER & TIM_CCER_CC2E)  mask |= (1U << 1U);
     if (TIM1->CCER & TIM_CCER_CC3E)  mask |= (1U << 2U);
     if (TIM2->CCER & TIM_CCER_CC1E)  mask |= (1U << 3U);

    return mask;
}

uint16_t pwm_get_mask(void)
{
    uint16_t mask = 0U;

    if (pwm_status.pwm0 != 0U) mask |= (1U << 0U);
    if (pwm_status.pwm1 != 0U) mask |= (1U << 1U);
    if (pwm_status.pwm2 != 0U) mask |= (1U << 2U);
    if (pwm_status.pwm3 != 0U) mask |= (1U << 3U);

    return mask;
}

uint8_t pwm_get_state(uint8_t pwm_number)
{
    if (pwm_number >= pwm_max)
    {
        return 0U;
    }

    return (pwm_get_duty(pwm_number) != 0U) ? 1U : 0U;
}