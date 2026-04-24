#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

#ifdef __cplusplus
extern "C"
{
#endif

    // ---------------------------------------------------------------
    // IDs dos itens de manutenção
    // ---------------------------------------------------------------
    typedef enum
    {
        MAINT_OIL = 0,     // Troca de Óleo
        MAINT_TIMING_BELT, // Correia Dentada
        MAINT_TIRES,       // Pneus (rodízio)
        MAINT_BRAKE_FLUID, // Fluido de Freio
        MAINT_AIR_FILTER,  // Filtro de Ar
        MAINT_SPARK_PLUGS, // Velas de Ignição
        MAINT_COOLANT,     // Fluido de Arrefecimento
        MAINT_COUNT
    } maint_id_t;

    extern const char *MAINT_NAMES[MAINT_COUNT];
    extern const int32_t MAINT_DEFAULT_INTERVAL_KM[MAINT_COUNT];

    // ---------------------------------------------------------------
    // Estrutura de um item de manutenção
    // ---------------------------------------------------------------
    typedef struct
    {
        int32_t last_km;     // KM da última manutenção
        int32_t interval_km; // Intervalo em km
        uint16_t last_day;   // 1-31
        uint16_t last_month; // 1-12
        uint16_t last_year;  // ex: 2025
        bool alert_active;
        bool valid; // já foi registrado ao menos 1x
    } maint_item_t;

    // ---------------------------------------------------------------
    // Dados completos do veículo — persistido no SD como JSON
    // ---------------------------------------------------------------
    typedef struct
    {
        uint32_t magic;   // 0xDEAD1234 para validação
        int32_t total_km; // Odômetro atual
        maint_item_t items[MAINT_COUNT];
    } vehicle_data_t;

#define VEHICLE_DATA_MAGIC 0xDEAD1234

    // ---------------------------------------------------------------
    // Status calculado (não persistido)
    // ---------------------------------------------------------------
    typedef enum
    {
        MAINT_STATUS_OK,
        MAINT_STATUS_WARN, // <= 10% do intervalo restante
        MAINT_STATUS_DUE,  // Vencido
    } maint_status_t;

    typedef struct
    {
        maint_status_t status;
        int32_t km_remaining; // Negativo = já vencido
        int32_t next_km;
    } maint_calc_t;

// ---------------------------------------------------------------
// Entrada de histórico (usada para popular a UI)
// ---------------------------------------------------------------
#define MAINT_LOG_MAX_VISIBLE 50 // máximo de entradas carregadas na memória

    typedef struct
    {
        char date[12]; // "DD/MM/AAAA"
        char item[20]; // nome do item
        char km[10];   // km como string
    } maint_log_entry_t;

// ---------------------------------------------------------------
// Caminhos no SD
// ---------------------------------------------------------------
#define SD_MAINT_DATA_PATH "/maint_data.json"
#define SD_MAINT_LOG_PATH "/maint_log.json"

    // ---------------------------------------------------------------
    // API — lógica
    // ---------------------------------------------------------------
    void maint_init_defaults(vehicle_data_t *vd);
    maint_calc_t maint_calc(const maint_item_t *item, int32_t current_km);
    void maint_register(maint_item_t *item, int32_t current_km,
                        uint16_t day, uint16_t month, uint16_t year);
    uint32_t maint_check_alerts(const vehicle_data_t *vd);

    // ---------------------------------------------------------------
    // API — SD card (JSON via ArduinoJSON)
    // Presupõe que SD.begin() já foi chamado na aplicação.
    // ---------------------------------------------------------------

    /** Salva vehicle_data_t em /maint_data.json */
    esp_err_t maint_sd_save(const vehicle_data_t *vd);

    /** Carrega /maint_data.json → vd. Retorna ESP_ERR_NOT_FOUND na 1ª vez. */
    esp_err_t maint_sd_load(vehicle_data_t *vd);

    /** Apaga /maint_data.json */
    esp_err_t maint_sd_erase(void);

    /**
     * Acrescenta uma entrada ao histórico em /maint_log.json.
     * Entradas mais recentes ficam no início do array (newest-first).
     */
    esp_err_t maint_sd_log_entry(maint_id_t id, int32_t km,
                                 uint16_t day, uint16_t month, uint16_t year);

    /**
     * Lê as últimas `max_entries` entradas do log para `out`.
     * `count` recebe o número real de entradas lidas.
     */
    esp_err_t maint_sd_read_log(maint_log_entry_t *out,
                                uint8_t max_entries, uint8_t *count);

    /** Apaga todo o histórico (/maint_log.json) */
    esp_err_t maint_sd_clear_log(void);

#ifdef __cplusplus
}
#endif

#endif // MAINTENANCE_H