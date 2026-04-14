#ifndef SETTINGS_H
#define SETTINGS_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#define SETTINGS_MAGIC 0xCAFE5E77

typedef struct
{
    uint32_t magic;
    uint8_t units;         // 0=km, 1=milhas
    uint8_t brightness;    // 20-100
    uint16_t screen_off_s; // 0=nunca, 30, 60, 120, 300
    bool alert_sound;      // futuro: buzzer
    uint8_t alert_type;    // 0=visual, 1=sonoro, 2=ambos
    bool led_enabled;
    uint8_t led_brightness; // 0-100
    uint8_t led_r, led_g, led_b;
    uint8_t led_mode;  // 0=fixo, 1=respirando, 2=reativo, 3=carro
    uint8_t led_speed; // 10-100
} app_settings_t;

void settings_defaults(app_settings_t *s);
esp_err_t settings_save(const app_settings_t *s);
esp_err_t settings_load(app_settings_t *s);

#endif