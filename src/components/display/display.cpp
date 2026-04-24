#include "display.h"
#include "driver/spi_master.h"
#include "driver/gpio.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ===============================================================
// Font 5x7 (ASCII 32–122)
// ===============================================================
static const uint8_t font5x7[][5] = {
    {0x00, 0x00, 0x00, 0x00, 0x00},
    {0x00, 0x00, 0x5F, 0x00, 0x00},
    {0x00, 0x07, 0x00, 0x07, 0x00},
    {0x14, 0x7F, 0x14, 0x7F, 0x14},
    {0x24, 0x2A, 0x7F, 0x2A, 0x12},
    {0x23, 0x13, 0x08, 0x64, 0x62},
    {0x36, 0x49, 0x55, 0x22, 0x50},
    {0x00, 0x05, 0x03, 0x00, 0x00},
    {0x00, 0x1C, 0x22, 0x41, 0x00},
    {0x00, 0x41, 0x22, 0x1C, 0x00},
    {0x14, 0x08, 0x3E, 0x08, 0x14},
    {0x08, 0x08, 0x3E, 0x08, 0x08},
    {0x00, 0x50, 0x30, 0x00, 0x00},
    {0x08, 0x08, 0x08, 0x08, 0x08},
    {0x00, 0x60, 0x60, 0x00, 0x00},
    {0x20, 0x10, 0x08, 0x04, 0x02},
    {0x3E, 0x51, 0x49, 0x45, 0x3E},
    {0x00, 0x42, 0x7F, 0x40, 0x00},
    {0x42, 0x61, 0x51, 0x49, 0x46},
    {0x21, 0x41, 0x45, 0x4B, 0x31},
    {0x18, 0x14, 0x12, 0x7F, 0x10},
    {0x27, 0x45, 0x45, 0x45, 0x39},
    {0x3C, 0x4A, 0x49, 0x49, 0x30},
    {0x01, 0x71, 0x09, 0x05, 0x03},
    {0x36, 0x49, 0x49, 0x49, 0x36},
    {0x06, 0x49, 0x49, 0x29, 0x1E},
    {0x00, 0x36, 0x36, 0x00, 0x00},
    {0x00, 0x56, 0x36, 0x00, 0x00},
    {0x08, 0x14, 0x22, 0x41, 0x00},
    {0x14, 0x14, 0x14, 0x14, 0x14},
    {0x00, 0x41, 0x22, 0x14, 0x08},
    {0x02, 0x01, 0x51, 0x09, 0x06},
    {0x32, 0x49, 0x79, 0x41, 0x3E},
    {0x7E, 0x11, 0x11, 0x11, 0x7E},
    {0x7F, 0x49, 0x49, 0x49, 0x36},
    {0x3E, 0x41, 0x41, 0x41, 0x22},
    {0x7F, 0x41, 0x41, 0x22, 0x1C},
    {0x7F, 0x49, 0x49, 0x49, 0x41},
    {0x7F, 0x09, 0x09, 0x09, 0x01},
    {0x3E, 0x41, 0x49, 0x49, 0x7A},
    {0x7F, 0x08, 0x08, 0x08, 0x7F},
    {0x00, 0x41, 0x7F, 0x41, 0x00},
    {0x20, 0x40, 0x41, 0x3F, 0x01},
    {0x7F, 0x08, 0x14, 0x22, 0x41},
    {0x7F, 0x40, 0x40, 0x40, 0x40},
    {0x7F, 0x02, 0x04, 0x02, 0x7F},
    {0x7F, 0x04, 0x08, 0x10, 0x7F},
    {0x3E, 0x41, 0x41, 0x41, 0x3E},
    {0x7F, 0x09, 0x09, 0x09, 0x06},
    {0x3E, 0x41, 0x51, 0x21, 0x5E},
    {0x7F, 0x09, 0x19, 0x29, 0x46},
    {0x46, 0x49, 0x49, 0x49, 0x31},
    {0x01, 0x01, 0x7F, 0x01, 0x01},
    {0x3F, 0x40, 0x40, 0x40, 0x3F},
    {0x1F, 0x20, 0x40, 0x20, 0x1F},
    {0x3F, 0x40, 0x38, 0x40, 0x3F},
    {0x63, 0x14, 0x08, 0x14, 0x63},
    {0x07, 0x08, 0x70, 0x08, 0x07},
    {0x61, 0x51, 0x49, 0x45, 0x43},
    {0x00, 0x7F, 0x41, 0x41, 0x00},
    {0x02, 0x04, 0x08, 0x10, 0x20},
    {0x00, 0x41, 0x41, 0x7F, 0x00},
    {0x04, 0x02, 0x01, 0x02, 0x04},
    {0x40, 0x40, 0x40, 0x40, 0x40},
    {0x00, 0x01, 0x02, 0x04, 0x00},
    {0x20, 0x54, 0x54, 0x54, 0x78},
    {0x7F, 0x48, 0x44, 0x44, 0x38},
    {0x38, 0x44, 0x44, 0x44, 0x20},
    {0x38, 0x44, 0x44, 0x48, 0x7F},
    {0x38, 0x54, 0x54, 0x54, 0x18},
    {0x08, 0x7E, 0x09, 0x01, 0x02},
    {0x0C, 0x52, 0x52, 0x52, 0x3E},
    {0x7F, 0x08, 0x04, 0x04, 0x78},
    {0x00, 0x44, 0x7D, 0x40, 0x00},
    {0x20, 0x40, 0x44, 0x3D, 0x00},
    {0x7F, 0x10, 0x28, 0x44, 0x00},
    {0x00, 0x41, 0x7F, 0x40, 0x00},
    {0x7C, 0x04, 0x18, 0x04, 0x78},
    {0x7C, 0x08, 0x04, 0x04, 0x78},
    {0x38, 0x44, 0x44, 0x44, 0x38},
    {0x7C, 0x14, 0x14, 0x14, 0x08},
    {0x08, 0x14, 0x14, 0x18, 0x7C},
    {0x7C, 0x08, 0x04, 0x04, 0x08},
    {0x48, 0x54, 0x54, 0x54, 0x20},
    {0x04, 0x3F, 0x44, 0x40, 0x20},
    {0x3C, 0x40, 0x40, 0x20, 0x7C},
    {0x1C, 0x20, 0x40, 0x20, 0x1C},
    {0x3C, 0x40, 0x30, 0x40, 0x3C},
    {0x44, 0x28, 0x10, 0x28, 0x44},
    {0x0C, 0x50, 0x50, 0x50, 0x3C},
    {0x44, 0x64, 0x54, 0x4C, 0x44},
};

// ===============================================================
// Handles SPI
// ===============================================================
static spi_device_handle_t spi_display;
static spi_device_handle_t spi_touch;

// Buffer DMA em DRAM — obrigatório pois flash não é acessível pelo DMA
// Tamanho = 80px × 2B × 16 linhas = 2560 bytes
// Suporta char scale até 6 (30×42×2=2520 bytes) em modo char
#define DMA_CHUNK_BYTES 2560
static uint8_t DMA_ATTR dma_buf[DMA_CHUNK_BYTES];

// ===============================================================
// DISPLAY — funções internas
// ===============================================================
static inline void disp_dc(uint8_t v) { gpio_set_level(DISPLAY_PIN_DC, v); }

static void disp_write_byte(uint8_t b)
{
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &b;
    spi_device_polling_transmit(spi_display, &t);
}

static void disp_write_word(uint16_t w)
{
    uint8_t buf[2] = {
        (uint8_t)(w >> 8),
        (uint8_t)(w & 0xFF)};
    spi_transaction_t t = {};
    t.length = 16;
    t.tx_buffer = buf;
    spi_device_polling_transmit(spi_display, &t);
}

static void ili_cmd(uint8_t c)
{
    disp_dc(0);
    disp_write_byte(c);
}
static void ili_dat(uint8_t d)
{
    disp_dc(1);
    disp_write_byte(d);
}

static void ili_set_window(uint16_t x0, uint16_t y0, uint16_t x1, uint16_t y1)
{
    ili_cmd(0x2A);
    disp_dc(1);
    disp_write_word(x0);
    disp_write_word(x1);
    ili_cmd(0x2B);
    disp_dc(1);
    disp_write_word(y0);
    disp_write_word(y1);
    ili_cmd(0x2C);
    disp_dc(1);
}

// Envia dma_buf via DMA — CPU bloqueia no semáforo, cedendo ao scheduler
static void dma_send(uint32_t byte_count)
{
    spi_transaction_t t = {};
    t.length = byte_count * 8;
    t.tx_buffer = dma_buf;
    t.rx_buffer = NULL;
    spi_device_transmit(spi_display, &t);
}

static void ili9341_init_registers(void)
{
    ili_cmd(0x01);
    vTaskDelay(pdMS_TO_TICKS(150));
    ili_cmd(0xCB);
    ili_dat(0x39);
    ili_dat(0x2C);
    ili_dat(0x00);
    ili_dat(0x34);
    ili_dat(0x02);
    ili_cmd(0xCF);
    ili_dat(0x00);
    ili_dat(0xC1);
    ili_dat(0x30);
    ili_cmd(0xE8);
    ili_dat(0x85);
    ili_dat(0x00);
    ili_dat(0x78);
    ili_cmd(0xEA);
    ili_dat(0x00);
    ili_dat(0x00);
    ili_cmd(0xED);
    ili_dat(0x64);
    ili_dat(0x03);
    ili_dat(0x12);
    ili_dat(0x81);
    ili_cmd(0xF7);
    ili_dat(0x20);
    ili_cmd(0xC0);
    ili_dat(0x23);
    ili_cmd(0xC1);
    ili_dat(0x10);
    ili_cmd(0xC5);
    ili_dat(0x3E);
    ili_dat(0x28);
    ili_cmd(0xC7);
    ili_dat(0x86);
    ili_cmd(0x36);
    ili_dat(0x28); // MADCTL landscape BGR
    ili_cmd(0x3A);
    ili_dat(0x55); // RGB565
    ili_cmd(0xB1);
    ili_dat(0x00);
    ili_dat(0x18);
    ili_cmd(0xB6);
    ili_dat(0x08);
    ili_dat(0x82);
    ili_dat(0x27);
    ili_cmd(0xF2);
    ili_dat(0x00);
    ili_cmd(0x26);
    ili_dat(0x01);
    ili_cmd(0xE0);
    ili_dat(0x0F);
    ili_dat(0x31);
    ili_dat(0x2B);
    ili_dat(0x0C);
    ili_dat(0x0E);
    ili_dat(0x08);
    ili_dat(0x4E);
    ili_dat(0xF1);
    ili_dat(0x37);
    ili_dat(0x07);
    ili_dat(0x10);
    ili_dat(0x03);
    ili_dat(0x0E);
    ili_dat(0x09);
    ili_dat(0x00);
    ili_cmd(0xE1);
    ili_dat(0x00);
    ili_dat(0x0E);
    ili_dat(0x14);
    ili_dat(0x03);
    ili_dat(0x11);
    ili_dat(0x07);
    ili_dat(0x31);
    ili_dat(0xC1);
    ili_dat(0x48);
    ili_dat(0x08);
    ili_dat(0x0F);
    ili_dat(0x0C);
    ili_dat(0x31);
    ili_dat(0x36);
    ili_dat(0x0F);
    ili_cmd(0x11);
    vTaskDelay(pdMS_TO_TICKS(120));
    ili_cmd(0x29);
    vTaskDelay(pdMS_TO_TICKS(25));
}

// ===============================================================
// TOUCH — CS manual, SPI3
// ===============================================================
static inline void touch_cs_low(void) { gpio_set_level(TOUCH_PIN_CS, 0); }
static inline void touch_cs_high(void) { gpio_set_level(TOUCH_PIN_CS, 1); }

static uint8_t touch_spi_byte(uint8_t out)
{
    uint8_t rx = 0;
    spi_transaction_t t = {};
    t.length = 8;
    t.tx_buffer = &out;
    t.rx_buffer = &rx;
    spi_device_polling_transmit(spi_touch, &t);
    return rx;
}

static uint16_t xpt_read_channel(uint8_t cmd)
{
    touch_cs_low();
    touch_spi_byte(cmd);
    esp_rom_delay_us(10);
    uint8_t hi = touch_spi_byte(0x00);
    uint8_t lo = touch_spi_byte(0x00);
    touch_cs_high();
    return (((uint16_t)hi << 8) | lo) >> 3;
}

#define XPT_SAMPLES 8
static uint16_t xpt_median(uint8_t cmd)
{
    uint16_t buf[XPT_SAMPLES];
    for (int i = 0; i < XPT_SAMPLES; i++)
    {
        buf[i] = xpt_read_channel(cmd);
        esp_rom_delay_us(100);
    }
    for (int i = 0; i < XPT_SAMPLES - 1; i++)
        for (int j = i + 1; j < XPT_SAMPLES; j++)
            if (buf[i] > buf[j])
            {
                uint16_t t = buf[i];
                buf[i] = buf[j];
                buf[j] = t;
            }
    return buf[XPT_SAMPLES / 2];
}

// ===============================================================
// API PÚBLICA — Display
// ===============================================================

esp_err_t display_init(void)
{
    gpio_config_t io = {
        .pin_bit_mask = (1ULL << DISPLAY_PIN_DC) |
                        (1ULL << DISPLAY_PIN_BL) |
                        (1ULL << LED_ONBOARD),
        .mode = GPIO_MODE_OUTPUT,
    };
    gpio_config(&io);
    gpio_set_level(DISPLAY_PIN_BL, 1);
    gpio_set_level(LED_ONBOARD, 0);

    spi_bus_config_t bus = {
        .mosi_io_num = DISPLAY_PIN_MOSI,
        .miso_io_num = DISPLAY_PIN_MISO,
        .sclk_io_num = DISPLAY_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = DISPLAY_WIDTH * DISPLAY_HEIGHT * 2,
    };
    esp_err_t err = spi_bus_initialize(SPI2_HOST, &bus, SPI_DMA_CH_AUTO);
    if (err != ESP_OK)
        return err;

    spi_device_interface_config_t dev = {};
    dev.clock_speed_hz = 26 * 1000 * 1000;
    dev.mode = 0;
    dev.spics_io_num = DISPLAY_PIN_CS;
    dev.queue_size = 7;
    err = spi_bus_add_device(SPI2_HOST, &dev, &spi_display);
    if (err != ESP_OK)
        return err;

    ili9341_init_registers();
    return ESP_OK;
}

// ---------------------------------------------------------------
// display_fill — DMA puro
// Pré-preenche dma_buf com a cor e envia em chunks.
// spi_device_transmit() bloqueia num semáforo → scheduler roda → IDLE reseta WDT
// ---------------------------------------------------------------
void display_fill(uint16_t color)
{
    uint8_t hi = color >> 8, lo = color & 0xFF;

    // Pré-preenche o buffer inteiro de uma vez
    for (int i = 0; i < DMA_CHUNK_BYTES; i += 2)
    {
        dma_buf[i] = hi;
        dma_buf[i + 1] = lo;
    }

    ili_set_window(0, 0, DISPLAY_WIDTH - 1, DISPLAY_HEIGHT - 1);

    uint32_t total = (uint32_t)DISPLAY_WIDTH * DISPLAY_HEIGHT * 2; // 153.600 bytes
    uint32_t sent = 0;
    while (sent < total)
    {
        uint32_t chunk = total - sent;
        if (chunk > DMA_CHUNK_BYTES)
            chunk = DMA_CHUNK_BYTES;
        dma_send(chunk); // cede ao scheduler durante a transferência
        sent += chunk;
    }
}

void display_draw_pixel(uint16_t x, uint16_t y, uint16_t color)
{
    if (x >= DISPLAY_WIDTH || y >= DISPLAY_HEIGHT)
        return;
    ili_set_window(x, y, x, y);
    disp_write_word(color);
}

// ---------------------------------------------------------------
// display_draw_rect — DMA para regiões >= 4 pixels
// Para 1-3 pixels usa polling (overhead DMA > custo de polling)
// ---------------------------------------------------------------
void display_draw_rect(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t color)
{
    if (!w || !h)
        return;

    uint32_t total = (uint32_t)w * h * 2;

    // Rects minúsculos (bordas de 1px, pixels isolados): polling é mais rápido
    if (total <= 8)
    {
        uint8_t hi = color >> 8, lo = color & 0xFF;
        ili_set_window(x, y, x + w - 1, y + h - 1);
        for (uint32_t i = 0; i < (uint32_t)(w * h); i++)
        {
            disp_write_byte(hi);
            disp_write_byte(lo);
        }
        return;
    }

    // Pré-preenche dma_buf com a cor (só precisa fazer 1x por rect,
    // o mesmo buffer é reutilizado em todos os chunks)
    uint8_t hi = color >> 8, lo = color & 0xFF;
    for (int i = 0; i < DMA_CHUNK_BYTES; i += 2)
    {
        dma_buf[i] = hi;
        dma_buf[i + 1] = lo;
    }

    ili_set_window(x, y, x + w - 1, y + h - 1);

    uint32_t sent = 0;
    while (sent < total)
    {
        uint32_t chunk = total - sent;
        if (chunk > DMA_CHUNK_BYTES)
            chunk = DMA_CHUNK_BYTES;
        dma_send(chunk);
        sent += chunk;
    }
}

void display_draw_rect_border(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                              uint16_t color, uint8_t t)
{
    if (!t)
        return;
    display_draw_rect(x, y, w, t, color);
    display_draw_rect(x, y + h - t, w, t, color);
    display_draw_rect(x, y + t, t, h - 2 * t, color);
    display_draw_rect(x + w - t, y + t, t, h - 2 * t, color);
}

void display_draw_line(int16_t x0, int16_t y0, int16_t x1, int16_t y1, uint16_t color)
{
    int16_t dx = abs(x1 - x0), dy = abs(y1 - y0);
    int16_t sx = x0 < x1 ? 1 : -1, sy = y0 < y1 ? 1 : -1, err = dx - dy;
    while (1)
    {
        display_draw_pixel(x0, y0, color);
        if (x0 == x1 && y0 == y1)
            break;
        int16_t e2 = 2 * err;
        if (e2 > -dy)
        {
            err -= dy;
            x0 += sx;
        }
        if (e2 < dx)
        {
            err += dx;
            y0 += sy;
        }
    }
}

// ---------------------------------------------------------------
// display_draw_char — UMA janela SPI + UM envio DMA por caractere
//
// Antes: 35 janelas + 70 bytes polling por char (scale=1)
//        315 janelas + 630 bytes polling por char (scale=3)
// Agora: 1 janela + 1 DMA por char — ~10–30x mais rápido
//
// Limite: scale <= 6 cabe no dma_buf (30×42×2 = 2520 < 2560)
// Scale >= 7 usa fallback coluna-a-coluna (improvável na prática)
// ---------------------------------------------------------------
void display_draw_char(uint16_t x, uint16_t y, char c,
                       uint16_t fg, uint16_t bg, uint8_t scale)
{
    if (c < 32 || c > 122)
        c = '?';
    const uint8_t *glyph = font5x7[c - 32];

    uint16_t char_w = 5 * scale;
    uint16_t char_h = 7 * scale;
    uint32_t px_bytes = (uint32_t)char_w * char_h * 2;

    if (px_bytes > DMA_CHUNK_BYTES)
    {
        // Fallback para scale muito grande (>= 7, raro)
        for (uint8_t col = 0; col < 5; col++)
            for (uint8_t row = 0; row < 7; row++)
                display_draw_rect(x + col * scale, y + row * scale, scale, scale,
                                  (glyph[col] & (1 << row)) ? fg : bg);
        return;
    }

    uint8_t fg_hi = fg >> 8, fg_lo = fg & 0xFF;
    uint8_t bg_hi = bg >> 8, bg_lo = bg & 0xFF;

    // Constrói buffer de pixels em ordem row-major (esquerda→direita, cima→baixo)
    // que é exatamente como o ILI9341 espera após ili_set_window
    uint8_t *p = dma_buf;
    for (uint8_t row = 0; row < 7; row++)
    {
        for (uint8_t sr = 0; sr < scale; sr++)
        { // linhas de escala vertical
            for (uint8_t col = 0; col < 5; col++)
            {
                uint8_t hi, lo;
                if (glyph[col] & (1 << row))
                {
                    hi = fg_hi;
                    lo = fg_lo;
                }
                else
                {
                    hi = bg_hi;
                    lo = bg_lo;
                }
                for (uint8_t sc = 0; sc < scale; sc++)
                { // colunas de escala
                    *p++ = hi;
                    *p++ = lo;
                }
            }
        }
    }

    ili_set_window(x, y, x + char_w - 1, y + char_h - 1);
    dma_send(px_bytes);
}

void display_draw_string(uint16_t x, uint16_t y, const char *str,
                         uint16_t fg, uint16_t bg, uint8_t scale)
{
    for (; *str; str++, x += 6 * scale)
        display_draw_char(x, y, *str, fg, bg, scale);
}

// ---------------------------------------------------------------
// display_draw_sprite — Transferência eficiente via DMA (inalterado)
// ---------------------------------------------------------------
void display_draw_sprite(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                         const uint16_t *pixels_rgb565)
{
    ili_set_window(x, y, x + w - 1, y + h - 1);

    const uint8_t *src = (const uint8_t *)pixels_rgb565;
    uint32_t total = (uint32_t)w * h * 2;
    uint32_t sent = 0;

    while (sent < total)
    {
        uint32_t chunk = total - sent;
        if (chunk > DMA_CHUNK_BYTES)
            chunk = DMA_CHUNK_BYTES;
        memcpy(dma_buf, src + sent, chunk);
        dma_send(chunk);
        sent += chunk;
    }
}

// ===============================================================
// API PÚBLICA — Touch XPT2046 (inalterado)
// ===============================================================

esp_err_t touch_init(void)
{
    gpio_config_t cs_io = {};

    cs_io.pin_bit_mask = (1ULL << TOUCH_PIN_CS),
    cs_io.mode = GPIO_MODE_OUTPUT,

    gpio_config(&cs_io);
    touch_cs_high();

    gpio_config_t irq_io = {
        .pin_bit_mask = (1ULL << TOUCH_PIN_IRQ),
        .mode = GPIO_MODE_INPUT,
        .pull_up_en = GPIO_PULLUP_ENABLE,
    };
    gpio_config(&irq_io);

    spi_bus_config_t bus = {
        .mosi_io_num = TOUCH_PIN_MOSI,
        .miso_io_num = TOUCH_PIN_MISO,
        .sclk_io_num = TOUCH_PIN_CLK,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 8,
    };
    esp_err_t err = spi_bus_initialize(SPI3_HOST, &bus, SPI_DMA_DISABLED);
    if (err != ESP_OK)
        return err;

    spi_device_interface_config_t dev = {};
    dev.clock_speed_hz = 1 * 1000 * 1000;
    dev.mode = 0;
    dev.spics_io_num = -1;
    dev.queue_size = 1;

    return spi_bus_add_device(SPI3_HOST, &dev, &spi_touch);
}

bool touch_is_pressed(void)
{
    return (gpio_get_level(TOUCH_PIN_IRQ) == 0);
}

bool touch_get_raw(uint16_t *x_raw, uint16_t *y_raw)
{
    if (!touch_is_pressed())
        return false;
    *x_raw = xpt_median(0xD0);
    *y_raw = xpt_median(0x90);
    return touch_is_pressed();
}

void touch_print_raw(void)
{
    uint16_t xr, yr;
    if (!touch_get_raw(&xr, &yr))
        return;
    printf("[RAW] X=%4d  Y=%4d\n", xr, yr);
}