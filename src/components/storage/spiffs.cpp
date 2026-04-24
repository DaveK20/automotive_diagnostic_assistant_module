#include "spiffs.h"
#include "esp_spiffs.h"
#include <stdio.h>

esp_err_t spiffs_init()
{
    esp_vfs_spiffs_conf_t conf = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true
    };

    return esp_vfs_spiffs_register(&conf);
}

esp_err_t spiffs_save_log(const char *data)
{
    FILE *f = fopen("/spiffs/maintenance_log.txt", "a");

    if (f == NULL)
        return ESP_FAIL;

    fprintf(f, "%s\n", data);

    fclose(f);

    return ESP_OK;
}

esp_err_t spiffs_read_log()
{
    FILE *f = fopen("/spiffs/maintenance_log.txt", "r");

    if (f == NULL)
        return ESP_FAIL;

    char line[128];

    while (fgets(line, sizeof(line), f) != NULL)
    {
        printf("%s", line);
    }

    fclose(f);

    return ESP_OK;
}