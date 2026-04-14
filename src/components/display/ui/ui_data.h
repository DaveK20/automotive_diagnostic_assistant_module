#ifndef UI_DATA_H
#define UI_DATA_H

#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------
// Dados OBD — preenchidos pelo app, consumidos pela UI
// ---------------------------------------------------------------
typedef struct
{
    uint16_t rpm;
    uint16_t speed_kmh;
    uint8_t temp_c;
    uint8_t fuel_pct; // 0–100
    int32_t total_km;
    uint16_t engine_hours;
    uint8_t readiness_pct; // 0–100 (saúde geral calculada pelo app)
} ui_obd_t;

// ---------------------------------------------------------------
// Status de um item de manutenção
// ---------------------------------------------------------------
typedef enum
{
    UI_MAINT_OK = 0,
    UI_MAINT_WARN = 1, // <= 10% de intervalo restante
    UI_MAINT_DUE = 2,  // Vencido
} ui_maint_status_t;

// ---------------------------------------------------------------
// Item de manutenção para exibição
// ---------------------------------------------------------------
#define UI_MAX_MAINT 7

typedef struct
{
    char name[20];
    ui_maint_status_t status;
    int32_t km_remaining; // Negativo = vencido
    int32_t last_km;
    int32_t next_km;
    int32_t interval_km;
    uint8_t progress_pct; // 0–100
    char last_date[12];   // "DD/MM/AAAA" ou ""
    bool valid;           // Já foi registrado ao menos uma vez
} ui_maint_row_t;

// ---------------------------------------------------------------
// Registro do histórico
// ---------------------------------------------------------------
#define UI_MAX_HIST 10

typedef struct
{
    char date[12]; // "DD/MM/AAAA"
    char item[20]; // Nome do item
    char km[10];   // "87.432"
} ui_hist_row_t;

// ---------------------------------------------------------------
// Alerta / lembrete
// ---------------------------------------------------------------
#define UI_MAX_ALERTS 6

typedef enum
{
    UI_ALERT_CRITICAL = 0, // Borda vermelha
    UI_ALERT_WARNING = 1,  // Borda laranja
    UI_ALERT_INFO = 2,     // Borda cinza
} ui_alert_level_t;

typedef struct
{
    ui_alert_level_t level;
    char text[52];
} ui_alert_row_t;

// ---------------------------------------------------------------
// Dataset completo — tudo que a UI precisa para renderizar
// O app preenche esse struct; a UI nunca toca em NVS/SPIFFS/OBD
// ---------------------------------------------------------------
typedef struct
{
    ui_obd_t obd;
    ui_maint_row_t maint[UI_MAX_MAINT];
    uint8_t maint_n;
    ui_hist_row_t hist[UI_MAX_HIST];
    uint8_t hist_n;
    ui_alert_row_t alerts[UI_MAX_ALERTS];
    uint8_t alert_n;
} ui_dataset_t;

#endif // UI_DATA_H