#include "settings.h"
#include "nvs_flash.h"
#include "nvs.h"
#include <string.h>

#define NVS_NS  "app_cfg"
#define NVS_KEY "settings"

void settings_defaults(app_settings_t *s) {
    memset(s, 0, sizeof(*s));
    s->magic         = SETTINGS_MAGIC;
    s->units         = 0;
    s->brightness    = 80;
    s->screen_off_s  = 0;
    s->alert_sound   = false;
    s->alert_type    = 0;
    s->led_enabled   = false;
    s->led_brightness= 50;
    s->led_r=0; s->led_g=200; s->led_b=0;
    s->led_mode      = 0;
    s->led_speed     = 50;
}

esp_err_t settings_save(const app_settings_t *s) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;
    err = nvs_set_blob(h, NVS_KEY, s, sizeof(*s));
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

esp_err_t settings_load(app_settings_t *s) {
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NS, NVS_READONLY, &h);
    if (err != ESP_OK) return err;
    size_t sz = sizeof(*s);
    err = nvs_get_blob(h, NVS_KEY, s, &sz);
    nvs_close(h);
    if (err != ESP_OK || s->magic != SETTINGS_MAGIC) {
        settings_defaults(s);
        return ESP_ERR_NOT_FOUND;
    }
    return ESP_OK;
}