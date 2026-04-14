#include "maintenance.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "esp_spiffs.h"
#include <string.h>
#include <stdio.h>

const char *MAINT_NAMES[MAINT_COUNT] = {
    "Troca de Oleo",
    "Correia Dentada",
    "Pneus",
    "Fluido de Freio",
    "Filtro de Ar",
    "Velas de Ignicao",
    "Arrefecimento",
};

const int32_t MAINT_DEFAULT_INTERVAL_KM[MAINT_COUNT] = {
    5000,   // Óleo
    60000,  // Correia
    20000,  // Pneus
    30000,  // Fluido freio
    15000,  // Filtro ar
    30000,  // Velas
    40000,  // Arrefecimento
};

#define NVS_NAMESPACE   "vehicle"
#define NVS_KEY_DATA    "vdata"
#define SPIFFS_LOG_PATH "/spiffs/maint_log.txt"
#define WARN_THRESHOLD  10  // % do intervalo restante para WARN

// ---------------------------------------------------------------
// Lógica
// ---------------------------------------------------------------

void maint_init_defaults(vehicle_data_t *vd)
{
    memset(vd, 0, sizeof(*vd));
    vd->magic    = VEHICLE_DATA_MAGIC;
    vd->total_km = 0;

    for (int i = 0; i < MAINT_COUNT; i++) {
        vd->items[i].interval_km   = MAINT_DEFAULT_INTERVAL_KM[i];
        vd->items[i].last_km       = 0;
        vd->items[i].alert_active  = false;
        vd->items[i].valid         = false;
    }
}

maint_calc_t maint_calc(const maint_item_t *item, int32_t current_km)
{
    maint_calc_t c;
    c.next_km      = item->last_km + item->interval_km;
    c.km_remaining = c.next_km - current_km;

    int32_t warn_threshold = item->interval_km * WARN_THRESHOLD / 100;

    if (c.km_remaining <= 0)
        c.status = MAINT_STATUS_DUE;
    else if (c.km_remaining <= warn_threshold)
        c.status = MAINT_STATUS_WARN;
    else
        c.status = MAINT_STATUS_OK;

    return c;
}

void maint_register(maint_item_t *item, int32_t current_km,
                    uint16_t day, uint16_t month, uint16_t year)
{
    item->last_km      = current_km;
    item->last_day     = day;
    item->last_month   = month;
    item->last_year    = year;
    item->alert_active = false;
    item->valid        = true;
}

uint32_t maint_check_alerts(const vehicle_data_t *vd)
{
    uint32_t mask = 0;
    for (int i = 0; i < MAINT_COUNT; i++) {
        if (!vd->items[i].valid) continue;
        maint_calc_t c = maint_calc(&vd->items[i], vd->total_km);
        if (c.status != MAINT_STATUS_OK)
            mask |= (1u << i);
    }
    return mask;
}

// ---------------------------------------------------------------
// Persistência — NVS
// Salva vehicle_data_t como blob binário.
// Máximo: ~500 bytes (bem dentro do limite NVS de ~4KB por entrada)
// ---------------------------------------------------------------

esp_err_t maint_nvs_save(const vehicle_data_t *vd)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_set_blob(h, NVS_KEY_DATA, vd, sizeof(*vd));
    if (err == ESP_OK)
        err = nvs_commit(h);

    nvs_close(h);
    return err;
}

esp_err_t maint_nvs_load(vehicle_data_t *vd)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK) return err;

    size_t sz = sizeof(*vd);
    err = nvs_get_blob(h, NVS_KEY_DATA, vd, &sz);
    nvs_close(h);

    // Valida magic para detectar dados corrompidos ou primeira inicialização
    if (err == ESP_OK && vd->magic != VEHICLE_DATA_MAGIC) {
        maint_init_defaults(vd);
        return ESP_ERR_NOT_FOUND;
    }
    return err;
}

esp_err_t maint_nvs_erase(void)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK) return err;

    err = nvs_erase_key(h, NVS_KEY_DATA);
    if (err == ESP_OK) nvs_commit(h);
    nvs_close(h);
    return err;
}

// ---------------------------------------------------------------
// Persistência — SPIFFS (log textual incremental)
// Cada registro: "DD/MM/AAAA;ITEM_NAME;KM\n"
// Pode ser lido/exportado para SD-card ou BT no futuro
// ---------------------------------------------------------------

esp_err_t maint_spiffs_log(maint_id_t id, int32_t km,
                            uint16_t day, uint16_t month, uint16_t year)
{
    if (id >= MAINT_COUNT) return ESP_ERR_INVALID_ARG;

    FILE *f = fopen(SPIFFS_LOG_PATH, "a");
    if (!f) return ESP_FAIL;

    fprintf(f, "%02u/%02u/%04u;%s;%ld\n",
            day, month, year, MAINT_NAMES[id], (long)km);
    fclose(f);
    return ESP_OK;
}

esp_err_t maint_spiffs_read_log(char *buf, size_t buf_len)
{
    FILE *f = fopen(SPIFFS_LOG_PATH, "r");
    if (!f) return ESP_FAIL;

    size_t read = fread(buf, 1, buf_len - 1, f);
    buf[read] = '\0';
    fclose(f);
    return ESP_OK;
}

esp_err_t maint_spiffs_clear_log(void)
{
    FILE *f = fopen(SPIFFS_LOG_PATH, "w");
    if (!f) return ESP_FAIL;
    fclose(f);
    return ESP_OK;
}