#include "nvs.h"
#include "nvs_flash.h"
#include "nvs.h"

#define NAMESPACE "vehicle"

esp_err_t storage_init()
{
    esp_err_t ret = nvs_flash_init();

    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        ret = nvs_flash_init();
    }

    return ret;
}

esp_err_t storage_save_km(int32_t km)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READWRITE, &handle);
    if (err != ESP_OK) return err;

    err = nvs_set_i32(handle, "km_total", km);

    if (err == ESP_OK)
        nvs_commit(handle);

    nvs_close(handle);

    return err;
}

esp_err_t storage_read_km(int32_t *km)
{
    nvs_handle_t handle;

    esp_err_t err = nvs_open(NAMESPACE, NVS_READONLY, &handle);
    if (err != ESP_OK) return err;

    err = nvs_get_i32(handle, "km_total", km);

    nvs_close(handle);

    return err;
}