#ifndef STORAGE_SPIFFS_H
#define STORAGE_SPIFFS_H

#include "esp_err.h"

esp_err_t spiffs_init();

esp_err_t spiffs_save_log(const char *data);

esp_err_t spiffs_read_log();

#endif