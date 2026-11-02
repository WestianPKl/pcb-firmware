#ifndef LED_H
#define LED_H

#include <stdint.h>
#include "stm32c5xx.h"

#define LED1_PIN 0U //PA0
#define LED2_PIN 1U //PA1
#define LED3_PIN 2U //PA2
#define LED4_PIN 3U //PA3

extern volatile uint8_t led1_state;
extern volatile uint8_t led2_state;
extern volatile uint8_t led3_state;
extern volatile uint8_t led4_state;

void led_init(void);
void led_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void led_off(GPIO_TypeDef *GPIOx, uint32_t pin);

void comm_led_activity(void);
void comm_led_process_100ms(void);

#endif // LED_H