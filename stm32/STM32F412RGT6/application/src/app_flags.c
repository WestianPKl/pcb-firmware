#include "app_flags.h"

volatile uint8_t ext_rtc_present = 0;
volatile uint8_t flash_present = 0;
volatile uint8_t display_present = 0;
volatile uint8_t sht40_present = 0;
volatile uint8_t bme280_present = 0;
volatile uint8_t ina226_present = 0;
volatile uint8_t adc_present = 0;
volatile uint8_t can_present = 0;
volatile uint8_t adc_was_present = 0;

volatile uint8_t fram_flags_toggle_flag = 0;

volatile uint8_t rtc_wakeup_flag = 0;
volatile uint8_t rtc_alarm_flag = 0;
volatile uint8_t rtc_tampstamp_flag = 0;

volatile uint8_t btn1_pressed = 0;
volatile uint8_t btn2_pressed = 0;

volatile uint8_t led1_state = 0;
volatile uint8_t led2_state = 0;
volatile uint8_t pb12_state = 0;
volatile uint8_t pc0_state = 0;
volatile uint8_t pc1_state = 0;
volatile uint8_t pc2_state = 0;
volatile uint8_t pc3_state = 0;
volatile uint8_t esp32_state = 0;

uint8_t rgb_r = 0;
uint8_t rgb_g = 0;
uint8_t rgb_b = 0;
uint8_t rgb_brightness = 0;

volatile uint8_t backlight_on = 1;
volatile uint8_t backlight_toggle_flag = 0;
volatile uint8_t backlight_timer = 0;

volatile uint8_t bme280_error_flag = 1;
volatile uint8_t sht40_error_flag = 1;

measurement_sht40_t measurement_sht40 = {0};
measurement_bme280_t measurement_bme280 = {0};
datetime_t datetime = {0};