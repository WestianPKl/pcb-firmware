#ifndef E_PAPER_H
#define E_PAPER_H

#include <stdint.h>
#include "stm32f7xx.h"

#define DC_PIN 13U // PD13
#define RST_PIN 14U // PD14
#define BUSY_PIN 15U // PD15

#define EPD_1IN64G_WIDTH       168
#define EPD_1IN64G_HEIGHT      168

#define  EPD_1IN64G_BLACK   0x0 
#define  EPD_1IN64G_WHITE   0x1
#define  EPD_1IN64G_YELLOW  0x2
#define  EPD_1IN64G_RED     0x3

void e_paper_init(void);
void e_paper_clear(uint8_t color);
void e_paper_display(const uint8_t *image);
void e_paper_sleep(void);

#endif // E_PAPER_H
