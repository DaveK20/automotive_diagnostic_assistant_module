#include <stdio.h>
#include <string.h>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_spiffs.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "components/display/display.h"
#include "components/display/calibration.h"
#include "components/maintenance/maintenance.h"
#include "components/display/ui/ui_data.h"
#include "components/display/ui/ui.h"

// ---------------------------------------------------------------
// Intervalo de polling OBD
// ---------------------------------------------------------------
#define OBD_POLL_MS 300
#define TOUCH_POLL_MS 20

// ---------------------------------------------------------------
// Dados OBD (voláteis, escritos pela task OBD)
// ---------------------------------------------------------------
static volatile struct
{
    uint16_t rpm;
    uint16_t speed_kmh;
    uint8_t temp_c;
    uint8_t fuel_pct;
    int32_t km_total;
    uint16_t engine_hours;
} g_obd = {0};

// ---------------------------------------------------------------
// Preenche um ui_maint_row_t a partir de um maint_item_t
// ---------------------------------------------------------------
static void fill_maint_row(ui_maint_row_t *row, const maint_item_t *item,
                           maint_id_t id, int32_t current_km)
{
    strncpy(row->name, MAINT_NAMES[id], sizeof(row->name) - 1);
    row->name[sizeof(row->name) - 1] = '\0';

    maint_calc_t c = maint_calc(item, current_km);

    switch (c.status)
    {
    case MAINT_STATUS_OK:
        row->status = UI_MAINT_OK;
        break;
    case MAINT_STATUS_WARN:
        row->status = UI_MAINT_WARN;
        break;
    case MAINT_STATUS_DUE:
        row->status = UI_MAINT_DUE;
        break;
    }

    row->km_remaining = c.km_remaining;
    row->last_km = item->last_km;
    row->next_km = c.next_km;
    row->interval_km = item->interval_km;
    row->valid = item->valid;

    if (item->interval_km > 0)
    {
        int32_t done = current_km - item->last_km;
        row->progress_pct = (uint8_t)((done * 100) / item->interval_km);
        if (row->progress_pct > 100)
            row->progress_pct = 100;
    }
    else
    {
        row->progress_pct = 0;
    }

    if (item->valid)
        snprintf(row->last_date, sizeof(row->last_date),
                 "%02u/%02u/%02u",
                 (unsigned int)(item->last_day % 100),
                 (unsigned int)(item->last_month % 100),
                 (unsigned int)(item->last_year % 100));
    else
        row->last_date[0] = '\0';
}

// ---------------------------------------------------------------
// Recalcula readiness_pct (0-100) baseado nos alertas de manutenção
// ---------------------------------------------------------------
static uint8_t calc_readiness(const vehicle_data_t *vd)
{
    int score = 100;
    for (int i = 0; i < MAINT_COUNT; i++)
    {
        if (!vd->items[i].valid)
            continue;
        maint_calc_t c = maint_calc(&vd->items[i], vd->total_km);
        if (c.status == MAINT_STATUS_DUE)
            score -= 15;
        else if (c.status == MAINT_STATUS_WARN)
            score -= 5;
    }
    if (score < 0)
        score = 0;
    return (uint8_t)score;
}

// ---------------------------------------------------------------
// Atualiza o dataset completo a partir do estado do app
// ---------------------------------------------------------------
static void update_dataset(ui_dataset_t *ds, const vehicle_data_t *vd)
{
    // OBD
    ds->obd.rpm = g_obd.rpm;
    ds->obd.speed_kmh = g_obd.speed_kmh;
    ds->obd.temp_c = g_obd.temp_c;
    ds->obd.fuel_pct = g_obd.fuel_pct;
    ds->obd.total_km = vd->total_km;
    ds->obd.engine_hours = g_obd.engine_hours;
    ds->obd.readiness_pct = calc_readiness(vd);

    // Manutenção
    ds->maint_n = MAINT_COUNT;
    for (int i = 0; i < MAINT_COUNT; i++)
        fill_maint_row(&ds->maint[i], &vd->items[i], (maint_id_t)i, vd->total_km);

    // Alertas
    ds->alert_n = 0;
    for (int i = 0; i < MAINT_COUNT && ds->alert_n < UI_MAX_ALERTS; i++)
    {
        maint_calc_t c = maint_calc(&vd->items[i], vd->total_km);
        if (c.status == MAINT_STATUS_DUE)
        {
            ui_alert_row_t *a = &ds->alerts[ds->alert_n++];
            a->level = UI_ALERT_CRITICAL;
            snprintf(a->text, sizeof(a->text),
                     "TROCA VENCIDA: %s (%ld KM)",
                     MAINT_NAMES[i], (long)(-c.km_remaining));
        }
        else if (c.status == MAINT_STATUS_WARN)
        {
            ui_alert_row_t *a = &ds->alerts[ds->alert_n++];
            a->level = UI_ALERT_WARNING;
            snprintf(a->text, sizeof(a->text),
                     "PROXIMA: %s em %ld KM",
                     MAINT_NAMES[i], (long)c.km_remaining);
        }
    }
}

// ---------------------------------------------------------------
// Callback de registro — chamado pela UI quando usuário confirma
// ---------------------------------------------------------------
static void on_register_maint(uint8_t idx, void *userdata)
{
    vehicle_data_t *vd = (vehicle_data_t *)userdata;
    if (idx >= MAINT_COUNT)
        return;

    // TODO: integrar RTC para data real
    maint_register(&vd->items[idx], vd->total_km, 1, 1, 2025);
    maint_nvs_save(vd);
    maint_spiffs_log((maint_id_t)idx, vd->total_km, 1, 1, 2025);
}

// ---------------------------------------------------------------
// Task OBD (simulada — substituir por obd_read_pid())
// ---------------------------------------------------------------
static void obd_task(void *arg)
{
    while (1)
    {
        // g_obd.rpm          = obd_get_rpm();
        // g_obd.speed_kmh    = obd_read_pid(0x0D);
        // g_obd.temp_c       = obd_read_pid(0x05) - 40;
        // g_obd.fuel_pct     = obd_read_pid(0x2F) * 100 / 255;
        vTaskDelay(pdMS_TO_TICKS(OBD_POLL_MS));
    }
}

// ---------------------------------------------------------------
// Task principal
// ---------------------------------------------------------------
static void main_task(void *arg)
{
    // NVS
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // SPIFFS
    esp_vfs_spiffs_conf_t spiffs_cfg = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_vfs_spiffs_register(&spiffs_cfg);

    display_init();
    touch_init();

    // Calibração
    touch_cal_t cal;
    if (calibration_load(&cal) != ESP_OK || !cal.valid)
        calibration_run(&cal);

    // Dados do veículo
    vehicle_data_t vd;
    if (maint_nvs_load(&vd) != ESP_OK)
        maint_init_defaults(&vd);

    // Popula histórico a partir do SPIFFS
    static ui_dataset_t ds;
    memset(&ds, 0, sizeof(ds));

    // Carrega histórico do log SPIFFS
    static char log_buf[512];
    if (maint_spiffs_read_log(log_buf, sizeof(log_buf)) == ESP_OK)
    {
        char *line = strtok(log_buf, "\n");
        while (line && ds.hist_n < UI_MAX_HIST)
        {
            ui_hist_row_t *h = &ds.hist[ds.hist_n];
            sscanf(line, "%11[^;];%19[^;];%9s", h->date, h->item, h->km);
            if (h->date[0])
                ds.hist_n++;
            line = strtok(NULL, "\n");
        }
    }

    // Inicializa UI
    ui_ctx_t ctx;
    update_dataset(&ds, &vd);
    ui_init(&ctx, &ds, on_register_maint, &vd);

    xTaskCreate(obd_task, "obd_task", 4096, NULL, 4, NULL);

    bool was_pressed = false;
    uint32_t last_obd_update = 0;

    while (1)
    {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

        // Atualiza dataset com novos dados OBD a cada intervalo
        if (now - last_obd_update >= OBD_POLL_MS)
        {
            vd.total_km = g_obd.km_total;
            update_dataset(&ds, &vd);
            ui_update_obd(&ctx);
            last_obd_update = now;
        }

        // Redesenha
        ui_draw(&ctx);

        // Toque
        uint16_t rx, ry;
        bool pressed = touch_get_raw(&rx, &ry);
        if (pressed && !was_pressed)
        {
            int16_t px, py;
            if (calibration_apply(&cal, rx, ry, &px, &py))
                ui_handle_touch(&ctx, px, py);
        }
        was_pressed = pressed;

        vTaskDelay(pdMS_TO_TICKS(TOUCH_POLL_MS));
    }
}

void app_main(void)
{
    xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
}