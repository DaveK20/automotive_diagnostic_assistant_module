#ifndef DISPLAY_H
#define DISPLAY_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ---------------------------------------------------------------
// Pinos Display ILI9341 — SPI2 (HSPI)
// ---------------------------------------------------------------
#define DISPLAY_PIN_MOSI 13
#define DISPLAY_PIN_MISO 12
#define DISPLAY_PIN_CLK 14
#define DISPLAY_PIN_CS 15
#define DISPLAY_PIN_DC 4 // D/C no GPIO 4
#define DISPLAY_PIN_RST -1
#define DISPLAY_PIN_BL 21

// LED onboard (GPIO 2 liberado pois D/C está no 4)
#define LED_ONBOARD 2

// ---------------------------------------------------------------
// Pinos Touch XPT2046 — SPI3 (VSPI), CS via GPIO
// ---------------------------------------------------------------
#define TOUCH_PIN_MOSI 32
#define TOUCH_PIN_MISO 22
#define TOUCH_PIN_CLK 25
#define TOUCH_PIN_CS 33
#define TOUCH_PIN_IRQ 23

// ---------------------------------------------------------------
// Resolução — landscape 320×240
// ---------------------------------------------------------------
#define DISPLAY_WIDTH 320
#define DISPLAY_HEIGHT 240

// ---------------------------------------------------------------
// Cores RGB565
// ---------------------------------------------------------------
#define COLOR_BLACK 0x0000
#define COLOR_WHITE 0xFFFF
#define COLOR_RED 0xF800
#define COLOR_GREEN 0x07E0
#define COLOR_BLUE 0x001F
#define COLOR_YELLOW 0xFFE0
#define COLOR_CYAN 0x07FF
#define COLOR_MAGENTA 0xF81F
#define COLOR_ORANGE 0xFD20
#define COLOR_GRAY 0x8410
#define COLOR_DARK_GREEN 0x03E0
#define COLOR_NAVY 0x000F
#define COLOR_DARK_GRAY 0x4208

#define RGB(r, g, b) ((uint16_t)((((r) & 0xF8) << 8) | (((g) & 0xFC) << 3) | ((b) >> 3)))

// ---------------------------------------------------------------
// API — Display
// ---------------------------------------------------------------
esp_err_t display_init(void);
void display_fill(uint16_t color);
void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color);
void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color);
void display_draw_rect_border(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                              uint16_t color, uint8_t thickness);
void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color);
void display_draw_char(uint16_t x, uint16_t y, char c,
                       uint16_t fg, uint16_t bg, uint8_t scale);
void display_draw_string(uint16_t x, uint16_t y, const char *str,
                         uint16_t fg, uint16_t bg, uint8_t scale);

// Desenha um sprite RGB565 em (x,y) com tamanho w×h via DMA.
// Atualiza APENAS a região do sprite — sem redesenhar a tela inteira.
// pixels_rgb565 pode estar em flash (a função copia para DRAM internamente).
void display_draw_sprite(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         const uint16_t *pixels_rgb565);

// ---------------------------------------------------------------
// API — Touch XPT2046
// ---------------------------------------------------------------
esp_err_t touch_init(void);
bool touch_is_pressed(void);
bool touch_get_raw(uint16_t *x_raw, uint16_t *y_raw);
void touch_print_raw(void);

#endif // DISPLAY_H