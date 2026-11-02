#ifndef DIGITAL_OUTPUT_H
#define DIGITAL_OUTPUT_H

#include <stdint.h>
#include "stm32c5xx.h"

#define OUTPUT_PHYSICAL_COUNT 12U

#define OUTPUT0_PIN 4U //PA4
#define OUTPUT1_PIN 5U //PA5
#define OUTPUT2_PIN 6U //PA6
#define OUTPUT3_PIN 7U //PA7
#define OUTPUT4_PIN 0U //PB0
#define OUTPUT5_PIN 1U //PB1
#define OUTPUT6_PIN 2U //PB2
#define OUTPUT7_PIN 3U //PB3
#define OUTPUT8_PIN 7U //PB7
#define OUTPUT9_PIN 8U //PB6
#define OUTPUT10_PIN 5U //PB5
#define OUTPUT11_PIN 4U //PB4

typedef struct {
    uint8_t output0;
    uint8_t output1;
    uint8_t output2;
    uint8_t output3;
    uint8_t output4;
    uint8_t output5;
    uint8_t output6;
    uint8_t output7;
    uint8_t output8;
    uint8_t output9;
    uint8_t output10;
    uint8_t output11;
} output_status_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint32_t pin;
} output_config_t;

static const output_config_t output_config[] =
{
    {GPIOA, OUTPUT0_PIN},
    {GPIOA, OUTPUT1_PIN},
    {GPIOA, OUTPUT2_PIN},
    {GPIOA, OUTPUT3_PIN},
    {GPIOB, OUTPUT4_PIN},
    {GPIOB, OUTPUT5_PIN},
    {GPIOB, OUTPUT6_PIN},
    {GPIOB, OUTPUT7_PIN},
    {GPIOB, OUTPUT8_PIN},
    {GPIOB, OUTPUT9_PIN},
    {GPIOB, OUTPUT10_PIN},
    {GPIOB, OUTPUT11_PIN},
};

extern volatile uint8_t output_max;

void digital_output_init(void);
void digital_output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void digital_output_off(GPIO_TypeDef *GPIOx, uint32_t pin);
uint8_t digital_output_set_state(uint8_t output_number, uint8_t state);
void digital_output_get_status(output_status_t *status);
uint16_t digital_output_get_gpio_mask(void);
uint16_t digital_output_get_mask(void);
uint8_t digital_output_get_state(uint8_t output_number);

#endif // DIGITAL_OUTPUT_H