#ifndef STORAGE_NVS_H
#define STORAGE_NVS_H

#include "esp_err.h"
#include <stdint.h>

esp_err_t storage_init();

esp_err_t storage_save_km(int32_t km);

esp_err_t storage_read_km(int32_t *km);

#endif