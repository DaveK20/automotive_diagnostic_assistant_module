#ifndef UI_DATA_H
#define UI_DATA_H

#include <stdint.h>
#include <stdbool.h>

// ---------------------------------------------------------------
// Dados OBD
// ---------------------------------------------------------------
typedef struct
{
    uint16_t rpm;
    uint16_t speed_kmh;
    uint8_t temp_c;
    uint8_t fuel_pct;
    int32_t total_km;
    uint16_t engine_hours;
    uint8_t readiness_pct; // calculado pelo app com base nos registros
} ui_obd_t;

// ---------------------------------------------------------------
// Status de manutenção — 4 estados distintos
// ---------------------------------------------------------------
typedef enum
{
    UI_MAINT_OK = 0,        // OK, dentro do intervalo
    UI_MAINT_WARN = 1,      // Amarelo: próximo do vencimento (<=10% restante)
    UI_MAINT_DUE = 2,       // Vermelho: intervalo vencido
    UI_MAINT_NO_RECORD = 3, // Amarelo: sem registro — nunca foi checado
} ui_maint_status_t;

// ---------------------------------------------------------------
// Item de manutenção
// ---------------------------------------------------------------
#define UI_MAX_MAINT 7

typedef struct
{
    char name[20];
    ui_maint_status_t status;
    int32_t km_remaining; // negativo = vencido
    int32_t last_km;
    int32_t next_km;
    int32_t interval_km;
    uint8_t progress_pct; // 0–100
    char last_date[14];   // "DD/MM/AAAA" ou ""
    bool valid;
} ui_maint_row_t;

// ---------------------------------------------------------------
// Histórico
// ---------------------------------------------------------------
#define UI_MAX_HIST 10

typedef struct
{
    char date[12];
    char item[20];
    char km[10];
} ui_hist_row_t;

// ---------------------------------------------------------------
// Alertas — pré-ordenados pelo app (críticos primeiro)
// ---------------------------------------------------------------
#define UI_MAX_ALERTS 14 // 7 DUE + 7 WARN/NO_RECORD

typedef enum
{
    UI_ALERT_CRITICAL = 0, // vermelho — DUE
    UI_ALERT_WARNING = 1,  // amarelo — WARN ou NO_RECORD
} ui_alert_level_t;

typedef struct
{
    ui_alert_level_t level;
    char text[60];
} ui_alert_row_t;

// ---------------------------------------------------------------
// Dataset completo — UI só lê, nunca escreve em storage
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