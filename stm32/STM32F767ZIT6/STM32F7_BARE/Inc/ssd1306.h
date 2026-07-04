#ifndef SSD1306_H
#define SSD1306_H

#include <stdint.h>
#include "stm32f7xx.h"
#include <stddef.h>

#define SSD1306_WIDTH   128U
#define SSD1306_HEIGHT  64U
#define SSD1306_PAGES   (SSD1306_HEIGHT / 8U)

void ssd1306_init(void);
void ssd1306_send_command(uint8_t command);
void ssd1306_send_data(const uint8_t *data, size_t length);
void ssd1306_clear_display(void);
void ssd1306_set_cursor(uint8_t col, uint8_t page);
void ssd1306_display_text(const char *text, uint8_t col, uint8_t page);


#endif // SSD1306_H
