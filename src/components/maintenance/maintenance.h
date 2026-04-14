#ifndef MAINTENANCE_H
#define MAINTENANCE_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

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

// Nomes para exibição no display (max 20 chars para scale=1)
extern const char *MAINT_NAMES[MAINT_COUNT];

// Intervalos padrão em km (ajustáveis por veículo)
extern const int32_t MAINT_DEFAULT_INTERVAL_KM[MAINT_COUNT];

// ---------------------------------------------------------------
// Estrutura de um item de manutenção
// Projetada para persistência direta em NVS (blob) ou SPIFFS (JSON)
// ---------------------------------------------------------------
typedef struct
{
    int32_t last_km;     // KM da última manutenção
    int32_t interval_km; // Intervalo em km
    uint16_t last_day;   // Dia (1-31)
    uint16_t last_month; // Mês (1-12)
    uint16_t last_year;  // Ano (ex: 2025)
    bool alert_active;   // Alerta disparado
    bool valid;          // Registro já foi salvo ao menos 1x
} maint_item_t;

// ---------------------------------------------------------------
// Dados completos do veículo
// Persistido como blob NVS ("vehicle_data") ou arquivo SPIFFS
// ---------------------------------------------------------------
typedef struct
{
    uint32_t magic;   // 0xDEAD1234 para validação
    int32_t total_km; // Odômetro atual
    maint_item_t items[MAINT_COUNT];
} vehicle_data_t;

#define VEHICLE_DATA_MAGIC 0xDEAD1234

// ---------------------------------------------------------------
// Status calculado de um item (não persistido)
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
// API
// ---------------------------------------------------------------

// Inicializa estrutura com valores padrão (sem apagar NVS)
void maint_init_defaults(vehicle_data_t *vd);

// Calcula status de um item em relação ao odômetro atual
maint_calc_t maint_calc(const maint_item_t *item, int32_t current_km);

// Registra uma manutenção (atualiza last_km, last_date)
void maint_register(maint_item_t *item, int32_t current_km,
                    uint16_t day, uint16_t month, uint16_t year);

// Verifica alertas — retorna bitmask (bit N = item N vencido/quase)
uint32_t maint_check_alerts(const vehicle_data_t *vd);

// ---------------------------------------------------------------
// Persistência — NVS (rápido, até ~4KB)
// ---------------------------------------------------------------
esp_err_t maint_nvs_save(const vehicle_data_t *vd);
esp_err_t maint_nvs_load(vehicle_data_t *vd);
esp_err_t maint_nvs_erase(void);

// ---------------------------------------------------------------
// Persistência — SPIFFS (histórico em texto, ilimitado)
// ---------------------------------------------------------------
esp_err_t maint_spiffs_log(maint_id_t id, int32_t km,
                           uint16_t day, uint16_t month, uint16_t year);
esp_err_t maint_spiffs_read_log(char *buf, size_t buf_len);
esp_err_t maint_spiffs_clear_log(void);

#endif // MAINTENANCE_H