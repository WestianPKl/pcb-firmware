#ifndef RELAY_H
#define RELAY_H

#include <stdint.h>
#include "stm32c5xx.h"

#define RELAY_PHYSICAL_COUNT 12U

#define RELAY0_PIN 4U //PA4
#define RELAY1_PIN 5U //PA5
#define RELAY2_PIN 6U //PA6
#define RELAY3_PIN 7U //PA7
#define RELAY4_PIN 0U //PB0
#define RELAY5_PIN 1U //PB1
#define RELAY6_PIN 2U //PB2
#define RELAY7_PIN 3U //PB3
#define RELAY8_PIN 4U //PB4
#define RELAY9_PIN 5U //PB5
#define RELAY10_PIN 6U //PB6
#define RELAY11_PIN 7U //PB7

typedef struct {
    uint8_t relay0;
    uint8_t relay1;
    uint8_t relay2;
    uint8_t relay3;
    uint8_t relay4;
    uint8_t relay5;
    uint8_t relay6;
    uint8_t relay7;
    uint8_t relay8;
    uint8_t relay9;
    uint8_t relay10;
    uint8_t relay11;
} relay_status_t;

typedef struct
{
    GPIO_TypeDef *port;
    uint32_t pin;
} relay_config_t;

static const relay_config_t relay_config[] =
{
    {GPIOA, RELAY0_PIN},
    {GPIOA, RELAY1_PIN},
    {GPIOA, RELAY2_PIN},
    {GPIOA, RELAY3_PIN},
    {GPIOB, RELAY4_PIN},
    {GPIOB, RELAY5_PIN},
    {GPIOB, RELAY6_PIN},
    {GPIOB, RELAY7_PIN},
    {GPIOB, RELAY8_PIN},
    {GPIOB, RELAY9_PIN},
    {GPIOB, RELAY10_PIN},
    {GPIOB, RELAY11_PIN},
};

extern volatile uint8_t relay_max;

void relay_init(void);
void relay_output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void relay_output_off(GPIO_TypeDef *GPIOx, uint32_t pin);
uint8_t relay_set_state(uint8_t relay_number, uint8_t state);
void relay_get_status(relay_status_t *status);
uint16_t relay_get_gpio_mask(void);
uint16_t relay_get_mask(void);
uint8_t relay_get_state(uint8_t relay_number);

#endif // RELAY_H