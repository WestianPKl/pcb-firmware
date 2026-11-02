#include <stdint.h>
#include "stm32f7xx.h"
#include "support.h"
#include "output.h"
#include "timer.h"
#include "i2c.h"
#include "input.h"
#include "lsm6d63.h"
#include "pi.h"

// static lsm6ds3_data_t imu_data = {0};

#define SAMPLES_COUNT 1000U

static volatile float rpm_samples[SAMPLES_COUNT];
static volatile float pwm_samples[SAMPLES_COUNT];
static volatile float error_samples[SAMPLES_COUNT];

static volatile uint32_t sample_index = 0U;

static pi_t motor_pi =
{
    .kp = 2.0f,
    .ki = 1.5f,
    .integral = 0.0f,
    .feedforward = 390.0f,
    .output_min = 0.0f,
    .output_max = 1000.0f
};

static volatile float target_rpm = 40.0f;
static volatile float motor_rpm = 0.0f;
static volatile float motor_error = 0.0f;
static volatile float motor_pwm = 0.0f;

static volatile int32_t motor_position = 0;

static uint8_t led1_state = 0;

static void clock_init_216mhz(void)
{
    RCC->CR |= RCC_CR_HSEBYP;
    RCC->CR |= RCC_CR_HSEON;
    while (!(RCC->CR & RCC_CR_HSERDY)) {}

    RCC->APB1ENR |= RCC_APB1ENR_PWREN;
    (void)RCC->APB1ENR;
    PWR->CR1 |= PWR_CR1_VOS;
    PWR->CR1 |= PWR_CR1_ODEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODRDY)) {}
    PWR->CR1 |= PWR_CR1_ODSWEN;
    while (!(PWR->CSR1 & PWR_CSR1_ODSWRDY)) {}

    FLASH->ACR = FLASH_ACR_LATENCY_7WS | FLASH_ACR_ARTEN | FLASH_ACR_PRFTEN;
    RCC->PLLCFGR =
        (4U   << RCC_PLLCFGR_PLLM_Pos) |
        (216U << RCC_PLLCFGR_PLLN_Pos) |
        (0U   << RCC_PLLCFGR_PLLP_Pos) |
        (9U   << RCC_PLLCFGR_PLLQ_Pos) |
        RCC_PLLCFGR_PLLSRC_HSE;

    RCC->CFGR &= ~(RCC_CFGR_HPRE | RCC_CFGR_PPRE1 | RCC_CFGR_PPRE2);
    RCC->CFGR |= RCC_CFGR_HPRE_DIV1 | RCC_CFGR_PPRE1_DIV4 | RCC_CFGR_PPRE2_DIV2;

    RCC->CR |= RCC_CR_PLLON;
    while (!(RCC->CR & RCC_CR_PLLRDY)) {}

    RCC->CFGR &= ~RCC_CFGR_SW;
    RCC->CFGR |= RCC_CFGR_SW_PLL;
    while ((RCC->CFGR & RCC_CFGR_SWS) != RCC_CFGR_SWS_PLL) {}

    SystemCoreClock = 216000000U;
}

int main(void)
{
    clock_init_216mhz();
    dwt_init();

    tim2_init();
    // i2c1_init();
    // lsm6d63_init();

    gpio_output_init(GPIOB, LED1);
    gpio_output_init(GPIOB, LED3);

    encoder_init();
    motor_pwm_init();

    output_on(GPIOF, DRV8876_PH_Pin);
    output_on(GPIOF, DRV8876_nSLEEP_Pin);

    delay_ms(10U);

    motor_pwm_set(0U);
    gpio_input_init(GPIOC, DRV8876_FAULT_Pin);

    pi_reset(&motor_pi);

    uint16_t previous_cnt = (uint16_t)TIM3->CNT;
    uint32_t previous_tick = tick_10ms;

    if (!drv8876_fault_flag && (GPIOC->IDR & (1U << DRV8876_FAULT_Pin)))
    {
        motor_pwm_set((uint16_t)motor_pi.feedforward);
    }
    else
    {
        drv8876_fault_flag = 1U;
        motor_pwm_set(0U);
    }

    uint32_t start_tick = tick_10ms;

	for(;;)
    {
        uint32_t now = tick_10ms;
        uint32_t elapsed = now - previous_tick;

        if ((GPIOC->IDR & (1U << DRV8876_FAULT_Pin)) == 0U)
        {
            drv8876_fault_flag = 1U;
        }

        if (drv8876_fault_flag)
        {
            motor_pwm_set(0U);
        }

        uint32_t run_time = tick_10ms - start_tick;
        if (run_time < 4000U)
        {
            target_rpm = 50.0f;
        }
        else if (run_time < 6000U)
        {
            target_rpm = 40.0f;
        }
        else
        {
            target_rpm = 50.0f;
        }

        if (elapsed >= 10U)
        {
            uint16_t current_cnt = (uint16_t)TIM3->CNT;
            int16_t delta = (int16_t)(uint16_t) (current_cnt - previous_cnt);

            previous_cnt = current_cnt;
            previous_tick = now;

            motor_position += delta;

            // RPM measurement
            motor_rpm = ((float)delta * 6000.0f) / (3840.0f * (float)elapsed);

            motor_error = target_rpm - motor_rpm;

            if (!drv8876_fault_flag)
            {
                // PI controller
                float dt = (float)elapsed * 0.01f;
                motor_pwm = pi_update(&motor_pi, target_rpm, motor_rpm, dt);

                if (sample_index < SAMPLES_COUNT)
                {
                    rpm_samples[sample_index] = motor_rpm;
                    pwm_samples[sample_index] = motor_pwm;
                    error_samples[sample_index] = motor_error;

                    sample_index++;
                }

                motor_pwm_set((uint16_t)(motor_pwm + 0.5f));
            }
            else
            {
                motor_pwm = 0.0f;
                motor_pwm_set(0U);
            }
        }

        if (drv8876_fault_flag)
        {
            motor_pwm = 0.0f;
            motor_pwm_set(0U);
            pi_reset(&motor_pi);
        }

        if (clock_tick_flag) {
            clock_tick_flag = 0;

            led1_state = !led1_state;
            if (led1_state) {
                output_on(GPIOB, LED1);
            } else {
                output_off(GPIOB, LED1);
            }
        }
    }
}
