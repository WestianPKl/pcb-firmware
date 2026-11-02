
#ifndef OUTPUT_H
#define OUTPUT_H

#include <stdint.h>
#include "stm32h5xx.h"

#define GREEN_LED_PIN 0U //PB0
#define YELLOW_LED_PIN 4U //PF4
#define RED_LED_PIN 4U //PG4

extern volatile uint8_t yellow_led_state;
extern volatile uint8_t red_led_state;
extern volatile uint8_t green_led_state;

void gpio_output_init(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void output_off(GPIO_TypeDef *GPIOx, uint32_t pin);

#endif // OUTPUT_H

