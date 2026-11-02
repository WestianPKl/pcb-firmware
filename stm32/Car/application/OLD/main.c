#include <stdint.h>
#include "stm32f7xx.h"
#include "support.h"
#include "output.h"
#include "timer.h"
#include "i2c.h"
#include "input.h"
#include "lsm6d63.h"
#include "pi.h"

static lsm6ds3_data_t imu_data = {0};

#define RPM_SAMPLE_COUNT 30U

static volatile float rpm_samples[RPM_SAMPLE_COUNT];
static volatile uint8_t rpm_sample_index = 0U;

static volatile float motor_rpm = 0.0f;
static volatile float target_rpm = 100.0f;
static volatile float motor_error = 0.0f;

static volatile int32_t motor_position = 0;

static uint8_t led1_state = 0;
// static uint8_t led3_state = 0;
// static uint8_t motor_state = 0;

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
    i2c1_init();
    lsm6d63_init();

    gpio_output_init(GPIOB, LED1);
    gpio_output_init(GPIOB, LED3);

    encoder_init();
    motor_pwm_init();

    output_on(GPIOF, DRV8876_PH_Pin);
    output_on(GPIOF, DRV8876_nSLEEP_Pin);

    delay_ms(10U);

    gpio_input_init(GPIOC, DRV8876_FAULT_Pin);

    if ((GPIOC->IDR & (1U << DRV8876_FAULT_Pin)) == 0U)
    {
        drv8876_fault_flag = 1U;
    }

    if (!drv8876_fault_flag)
    {
        motor_pwm_set(473U);
    }

    delay_ms(120000U); // Wait for 2 minutes before starting the main loop

    uint16_t previous_cnt = (uint16_t)TIM3->CNT;
    uint32_t previous_tick = tick_10ms;

	for(;;)
    {
        uint32_t now = tick_10ms;
        uint32_t elapsed = now - previous_tick;

        if ((elapsed >= 10U) &&
            (rpm_sample_index < RPM_SAMPLE_COUNT))
        {
            uint16_t current_cnt = (uint16_t)TIM3->CNT;
            int16_t delta = (int16_t)((uint16_t)current_cnt - previous_cnt);

            previous_cnt = current_cnt;
            previous_tick = now;
            motor_position += delta;
            motor_rpm = ((float)delta * 6000.0f) / (3840.0f * (float)elapsed);
            motor_error = target_rpm - motor_rpm;

            rpm_samples[rpm_sample_index] = motor_rpm;
            rpm_sample_index++;

            if (rpm_sample_index >= RPM_SAMPLE_COUNT)
            {
                motor_pwm_set(0U);
            }

            uint8_t result = lsm6ds3_read_data(&imu_data);

            if (result == 1U)
            {
                // New IMU data
            }
        }

        if (drv8876_fault_flag)
        {
            motor_pwm_set(0U);
        }

        if (clock_tick_flag) {
            clock_tick_flag = 0;

            led1_state = !led1_state;
            if (led1_state) {
                output_on(GPIOB, LED1);
            } else {
                output_off(GPIOB, LED1);
            }

            // if (motor_state == 0) {
            //     motor_pwm_set(0U);    // 0%
            //     motor_state = 1;
            // } else if (motor_state == 1) {
            //     motor_pwm_set(250U);  // 25%
            //     motor_state = 2;
            // } else if (motor_state == 2) {
            //     motor_pwm_set(500U);  // 50%
            //     motor_state = 3;
            // } else if (motor_state == 3) {
            //     motor_pwm_set(1000U); // 100%
            //     motor_state = 0;
            // }
        }
    }
}
