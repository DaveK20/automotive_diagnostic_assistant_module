/**
 * maintenance.cpp
 *
 * Persistência via SD card + ArduinoJSON (v6 ou v7).
 * Dois arquivos JSON no SD:
 *   /maint_data.json  — estado atual do veículo (odômetro + itens)
 *   /maint_log.json   — histórico completo de trocas (array, newest-first)
 *
 * Requer que SD.begin() tenha sido chamado antes de qualquer função sd_*.
 * Compatível com ESP32 Arduino (SD.h / SPI).
 */

#include "maintenance.h"
#include <ArduinoJson.h>
#include <string.h>
#include <stdio.h>
#include <stdlib.h>

// ---------------------------------------------------------------
// Tabelas
// ---------------------------------------------------------------
extern "C" const char *MAINT_NAMES[MAINT_COUNT] = {
    "Troca de Oleo",
    "Correia Dentada",
    "Pneus",
    "Fluido de Freio",
    "Filtro de Ar",
    "Velas de Ignicao",
    "Arrefecimento",
};

extern "C" const int32_t MAINT_DEFAULT_INTERVAL_KM[MAINT_COUNT] = {
    5000,  // Oleo
    60000, // Correia
    20000, // Pneus
    30000, // Fluido de freio
    15000, // Filtro de ar
    30000, // Velas
    40000, // Arrefecimento
};

#define WARN_THRESHOLD 10 // % restante para status WARN

// ---------------------------------------------------------------
// Logica (sem hardware)
// ---------------------------------------------------------------
extern "C" void maint_init_defaults(vehicle_data_t *vd)
{
    memset(vd, 0, sizeof(*vd));
    vd->magic = VEHICLE_DATA_MAGIC;
    vd->total_km = 0;
    for (int i = 0; i < MAINT_COUNT; i++)
    {
        vd->items[i].interval_km = MAINT_DEFAULT_INTERVAL_KM[i];
        vd->items[i].alert_active = false;
        vd->items[i].valid = false;
    }
}

extern "C" maint_calc_t maint_calc(const maint_item_t *item, int32_t current_km)
{
    maint_calc_t c;
    c.next_km = item->last_km + item->interval_km;
    c.km_remaining = c.next_km - current_km;
    int32_t warn_thr = item->interval_km * WARN_THRESHOLD / 100;

    if (c.km_remaining <= 0)
        c.status = MAINT_STATUS_DUE;
    else if (c.km_remaining <= warn_thr)
        c.status = MAINT_STATUS_WARN;
    else
        c.status = MAINT_STATUS_OK;
    return c;
}

extern "C" void maint_register(maint_item_t *item, int32_t current_km,
                               uint16_t day, uint16_t month, uint16_t year)
{
    item->last_km = current_km;
    item->last_day = day;
    item->last_month = month;
    item->last_year = year;
    item->alert_active = false;
    item->valid = true;
}

extern "C" uint32_t maint_check_alerts(const vehicle_data_t *vd)
{
    uint32_t mask = 0;
    for (int i = 0; i < MAINT_COUNT; i++)
    {
        if (!vd->items[i].valid)
            continue;
        maint_calc_t c = maint_calc(&vd->items[i], vd->total_km);
        if (c.status != MAINT_STATUS_OK)
            mask |= (1u << i);
    }
    return mask;
}

// ---------------------------------------------------------------
// Helpers FILE* — leitura/escrita via buffer heap
// ---------------------------------------------------------------

/** Le arquivo inteiro para buffer alocado em heap. Caller chama free(). */
static char *read_file_to_buf(const char *path, size_t *out_len)
{
    FILE *f = fopen(path, "r");
    if (!f)
        return NULL;

    fseek(f, 0, SEEK_END);
    long sz = ftell(f);
    rewind(f);

    if (sz <= 0)
    {
        fclose(f);
        return NULL;
    }

    char *buf = (char *)malloc((size_t)sz + 1);
    if (!buf)
    {
        fclose(f);
        return NULL;
    }

    size_t n = fread(buf, 1, (size_t)sz, f);
    buf[n] = '\0';
    fclose(f);

    if (out_len)
        *out_len = n;
    return buf;
}

/** Serializa doc e sobrescreve path. */
static bool write_doc_to_file(const char *path, const JsonDocument &doc)
{
    size_t len = measureJson(doc);
    char *buf = (char *)malloc(len + 1);
    if (!buf)
        return false;

    serializeJson(doc, buf, len + 1);

    FILE *f = fopen(path, "w");
    if (!f)
    {
        free(buf);
        return false;
    }

    fwrite(buf, 1, len, f);
    fclose(f);
    free(buf);
    return true;
}

/** Carrega /sd/maint_log.json em doc; inicializa entries vazio se falhar. */
static void load_log_doc(JsonDocument &doc)
{
    size_t len = 0;
    char *buf = read_file_to_buf(SD_MAINT_LOG_PATH, &len);
    if (!buf)
    {
        doc["entries"].to<JsonArray>();
        return;
    }
    DeserializationError err = deserializeJson(doc, buf, len);
    free(buf);

    if (err || !doc["entries"].is<JsonArray>())
    {
        doc.clear();
        doc["entries"].to<JsonArray>();
    }
}

// ---------------------------------------------------------------
// SD - vehicle_data (estado atual)
// ---------------------------------------------------------------
extern "C" esp_err_t maint_sd_save(const vehicle_data_t *vd)
{
    JsonDocument doc;
    doc["magic"] = (uint32_t)vd->magic;
    doc["total_km"] = vd->total_km;

    JsonArray arr = doc["items"].to<JsonArray>();
    for (int i = 0; i < MAINT_COUNT; i++)
    {
        const maint_item_t *it = &vd->items[i];
        JsonObject obj = arr.add<JsonObject>();
        obj["id"] = i;
        obj["name"] = MAINT_NAMES[i];
        obj["last_km"] = it->last_km;
        obj["interval_km"] = it->interval_km;
        obj["last_day"] = it->last_day;
        obj["last_month"] = it->last_month;
        obj["last_year"] = it->last_year;
        obj["alert_active"] = it->alert_active;
        obj["valid"] = it->valid;
    }

    return write_doc_to_file(SD_MAINT_DATA_PATH, doc) ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t maint_sd_load(vehicle_data_t *vd)
{
    size_t len = 0;
    char *buf = read_file_to_buf(SD_MAINT_DATA_PATH, &len);
    if (!buf)
    {
        maint_init_defaults(vd);
        return ESP_ERR_NOT_FOUND;
    }

    JsonDocument doc;
    DeserializationError err = deserializeJson(doc, buf, len);
    free(buf);

    if (err)
    {
        maint_init_defaults(vd);
        return ESP_FAIL;
    }

    uint32_t magic = doc["magic"] | (uint32_t)0;
    if (magic != VEHICLE_DATA_MAGIC)
    {
        maint_init_defaults(vd);
        return ESP_ERR_NOT_FOUND;
    }

    vd->magic = magic;
    vd->total_km = doc["total_km"] | (int32_t)0;

    for (int i = 0; i < MAINT_COUNT; i++)
        vd->items[i].interval_km = MAINT_DEFAULT_INTERVAL_KM[i];

    for (JsonObject obj : doc["items"].as<JsonArray>())
    {
        int id = obj["id"] | -1;
        if (id < 0 || id >= MAINT_COUNT)
            continue;
        maint_item_t *it = &vd->items[id];
        it->last_km = obj["last_km"] | (int32_t)0;
        it->interval_km = obj["interval_km"] | MAINT_DEFAULT_INTERVAL_KM[id];
        it->last_day = obj["last_day"] | (uint16_t)1;
        it->last_month = obj["last_month"] | (uint16_t)1;
        it->last_year = obj["last_year"] | (uint16_t)2025;
        it->alert_active = obj["alert_active"] | false;
        it->valid = obj["valid"] | false;
    }

    return ESP_OK;
}

extern "C" esp_err_t maint_sd_erase(void)
{
    remove(SD_MAINT_DATA_PATH);
    return ESP_OK;
}

// ---------------------------------------------------------------
// SD - log historico (newest-first)
// ---------------------------------------------------------------
extern "C" esp_err_t maint_sd_log_entry(maint_id_t id, int32_t km,
                                        uint16_t day, uint16_t month, uint16_t year)
{
    if (id >= MAINT_COUNT)
        return ESP_ERR_INVALID_ARG;

    JsonDocument doc;
    load_log_doc(doc);
    JsonArray old_arr = doc["entries"].as<JsonArray>();

    // Novo doc com entrada nova no inicio (newest-first)
    JsonDocument tmp;
    JsonArray new_arr = tmp["entries"].to<JsonArray>();

    char date_str[20], km_str[12];
    snprintf(date_str, sizeof(date_str), "%02u/%02u/%04u", day, month, year);
    snprintf(km_str, sizeof(km_str), "%ld", (long)km);

    JsonObject novo = new_arr.add<JsonObject>();
    novo["date"] = date_str;
    novo["item"] = MAINT_NAMES[id];
    novo["km"] = km_str;

    for (JsonObject e : old_arr)
        new_arr.add<JsonObject>() = e;

    return write_doc_to_file(SD_MAINT_LOG_PATH, tmp) ? ESP_OK : ESP_FAIL;
}

extern "C" esp_err_t maint_sd_read_log(maint_log_entry_t *out,
                                       uint8_t max_entries, uint8_t *count)
{
    *count = 0;
    JsonDocument doc;
    load_log_doc(doc);

    for (JsonObject e : doc["entries"].as<JsonArray>())
    {
        if (*count >= max_entries)
            break;
        maint_log_entry_t *row = &out[*count];
        strlcpy(row->date, e["date"] | "---", sizeof(row->date));
        strlcpy(row->item, e["item"] | "---", sizeof(row->item));
        strlcpy(row->km, e["km"] | "0", sizeof(row->km));
        (*count)++;
    }
    return ESP_OK;
}

extern "C" esp_err_t maint_sd_clear_log(void)
{
    remove(SD_MAINT_LOG_PATH);
    return ESP_OK;
}