#ifndef RELAY_H
#define RELAY_H

#include <stdint.h>
#include "stm32c5xx.h"

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

extern volatile uint8_t relay0_state;
extern volatile uint8_t relay1_state;
extern volatile uint8_t relay2_state;
extern volatile uint8_t relay3_state;
extern volatile uint8_t relay4_state;
extern volatile uint8_t relay5_state;
extern volatile uint8_t relay6_state;
extern volatile uint8_t relay7_state;
extern volatile uint8_t relay8_state;
extern volatile uint8_t relay9_state;
extern volatile uint8_t relay10_state;
extern volatile uint8_t relay11_state;

void relay_init(void);
void relay_output_on(GPIO_TypeDef *GPIOx, uint32_t pin);
void relay_output_off(GPIO_TypeDef *GPIOx, uint32_t pin);

#endif // RELAY_H