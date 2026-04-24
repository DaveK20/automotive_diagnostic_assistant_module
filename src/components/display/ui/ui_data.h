#ifndef UI_DATA_H
#define UI_DATA_H

#include <stdint.h>
#include <stdbool.h>
#include "components/settings/settings.h"
#include "components\maintenance\maintenance.h" // maint_log_entry_t, MAINT_LOG_MAX_VISIBLE

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
    uint8_t readiness_pct;
} ui_obd_t;

// ---------------------------------------------------------------
// Status de manutenção
// ---------------------------------------------------------------
typedef enum
{
    UI_MAINT_OK = 0,
    UI_MAINT_WARN,
    UI_MAINT_DUE,
    UI_MAINT_NO_RECORD,
} ui_maint_status_t;

#define UI_MAX_MAINT 7

typedef struct
{
    char name[20];
    ui_maint_status_t status;
    int32_t km_remaining;
    int32_t last_km;
    int32_t next_km;
    int32_t interval_km;
    uint8_t progress_pct;
    char last_date[14];
    bool valid;
} ui_maint_row_t;

// ---------------------------------------------------------------
// Histórico — lido do SD (newest-first)
// ---------------------------------------------------------------
#define UI_MAX_HIST 50 // suficiente para exibir todo o histórico do SD

typedef struct
{
    char date[12];
    char item[20];
    char km[10];
} ui_hist_row_t;

// ---------------------------------------------------------------
// Alertas — pré-ordenados (críticos primeiro)
// ---------------------------------------------------------------
#define UI_MAX_ALERTS 14

typedef enum
{
    UI_ALERT_CRITICAL = 0,
    UI_ALERT_WARNING = 1,
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
    app_settings_t settings;
} ui_dataset_t;

// ---------------------------------------------------------------
// Helper — popula hist[] a partir do log do SD
//
// Chame após maint_sd_read_log() em qualquer atualização de tela.
// Exemplo de uso na camada de aplicação:
//
//   maint_log_entry_t log[UI_MAX_HIST];
//   uint8_t n = 0;
//   maint_sd_read_log(log, UI_MAX_HIST, &n);
//   ui_dataset_populate_hist(&dataset, log, n);
// ---------------------------------------------------------------
static inline void ui_dataset_populate_hist(ui_dataset_t *d,
                                            const maint_log_entry_t *entries,
                                            uint8_t count)
{
    if (count > UI_MAX_HIST)
        count = UI_MAX_HIST;
    d->hist_n = count;
    for (uint8_t i = 0; i < count; i++)
    {
        // strncpy seguro — campos têm mesmas dimensões em ambas as structs
        __builtin_strncpy(d->hist[i].date, entries[i].date, sizeof(d->hist[i].date) - 1);
        d->hist[i].date[sizeof(d->hist[i].date) - 1] = '\0';

        __builtin_strncpy(d->hist[i].item, entries[i].item, sizeof(d->hist[i].item) - 1);
        d->hist[i].item[sizeof(d->hist[i].item) - 1] = '\0';

        __builtin_strncpy(d->hist[i].km, entries[i].km, sizeof(d->hist[i].km) - 1);
        d->hist[i].km[sizeof(d->hist[i].km) - 1] = '\0';
    }
}

#endif // UI_DATA_H