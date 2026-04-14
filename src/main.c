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
#include "components/settings/settings.h"
#include "components/display/ui/ui_data.h"
#include "components/display/ui/ui.h"

#define OBD_POLL_MS 300
#define TOUCH_POLL_MS 20

static volatile struct
{
    uint16_t rpm, speed_kmh;
    uint8_t temp_c, fuel_pct;
    int32_t km_total;
    uint16_t engine_hours;
} g_obd = {0};

// ---------------------------------------------------------------
// Readiness baseado nos registros de manutenção
// ---------------------------------------------------------------
static uint8_t calc_readiness(const vehicle_data_t *vd)
{
    const int pts = 100 / MAINT_COUNT;
    int score = 100;
    for (int i = 0; i < MAINT_COUNT; i++)
    {
        const maint_item_t *it = &vd->items[i];
        if (!it->valid)
        {
            score -= pts;
            continue;
        }
        maint_calc_t c = maint_calc(it, vd->total_km);
        if (c.status == MAINT_STATUS_DUE)
            score -= pts;
        else if (c.status == MAINT_STATUS_WARN)
            score -= pts / 2;
    }
    return (uint8_t)(score < 0 ? 0 : score);
}

// ---------------------------------------------------------------
// Preenche um ui_maint_row_t
// ---------------------------------------------------------------
static void fill_maint_row(ui_maint_row_t *row, const maint_item_t *item,
                           maint_id_t id, int32_t km)
{
    strncpy(row->name, MAINT_NAMES[id], sizeof(row->name) - 1);
    row->name[sizeof(row->name) - 1] = '\0';
    row->valid = item->valid;
    row->last_km = item->last_km;
    row->interval_km = item->interval_km;

    if (!item->valid)
    {
        row->status = UI_MAINT_NO_RECORD;
        row->km_remaining = 0;
        row->next_km = item->interval_km;
        row->progress_pct = 0;
        row->last_date[0] = '\0';
        return;
    }
    maint_calc_t c = maint_calc(item, km);
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
    row->next_km = c.next_km;
    if (item->interval_km > 0)
    {
        int32_t done = km - item->last_km;
        row->progress_pct = (uint8_t)((done * 100) / item->interval_km);
        if (row->progress_pct > 100)
            row->progress_pct = 100;
    }
    else
    {
        row->progress_pct = 0;
    }

    unsigned int day = item->last_day;
    unsigned int month = item->last_month;
    unsigned int year = item->last_year;

    // Limita valores (IMPORTANTE)
    if (day > 99)
        day = 99;
    if (month > 99)
        month = 99;
    if (year > 9999)
        year = 9999;

    snprintf(row->last_date, sizeof(row->last_date),
             "%02u/%02u/%04u",
             day, month, year);
}

// ---------------------------------------------------------------
// Alertas ordenados: DUE → WARN → NO_RECORD
// ---------------------------------------------------------------
static void build_alerts(ui_dataset_t *ds, const vehicle_data_t *vd)
{
    ds->alert_n = 0;
    // 1ª passagem: vencidos (vermelho)
    for (int i = 0; i < ds->maint_n && ds->alert_n < UI_MAX_ALERTS; i++)
    {
        const ui_maint_row_t *m = &ds->maint[i];
        if (m->status != UI_MAINT_DUE)
            continue;
        ui_alert_row_t *a = &ds->alerts[ds->alert_n++];
        a->level = UI_ALERT_CRITICAL;
        snprintf(a->text, sizeof(a->text),
                 "VENCIDO: %.20s (%ldKM)", m->name, (long)(-m->km_remaining));
    }
    // 2ª passagem: próximos e sem registro (amarelo)
    for (int i = 0; i < ds->maint_n && ds->alert_n < UI_MAX_ALERTS; i++)
    {
        const ui_maint_row_t *m = &ds->maint[i];
        if (m->status == UI_MAINT_WARN)
        {
            ui_alert_row_t *a = &ds->alerts[ds->alert_n++];
            a->level = UI_ALERT_WARNING;
            snprintf(a->text, sizeof(a->text),
                     "PROXIMO: %.20s em %ldKM", m->name, (long)m->km_remaining);
        }
        else if (m->status == UI_MAINT_NO_RECORD)
        {
            ui_alert_row_t *a = &ds->alerts[ds->alert_n++];
            a->level = UI_ALERT_WARNING;
            snprintf(a->text, sizeof(a->text), "SEM REGISTRO: %.20s", m->name);
        }
    }
}

// ---------------------------------------------------------------
// Atualiza dataset
// ---------------------------------------------------------------
static void update_dataset(ui_dataset_t *ds, const vehicle_data_t *vd,
                           const app_settings_t *s)
{
    ds->obd.rpm = g_obd.rpm;
    ds->obd.speed_kmh = g_obd.speed_kmh;
    ds->obd.temp_c = g_obd.temp_c;
    ds->obd.fuel_pct = g_obd.fuel_pct;
    ds->obd.total_km = vd->total_km;
    ds->obd.engine_hours = g_obd.engine_hours;
    ds->obd.readiness_pct = calc_readiness(vd);
    ds->maint_n = MAINT_COUNT;
    for (int i = 0; i < MAINT_COUNT; i++)
        fill_maint_row(&ds->maint[i], &vd->items[i], (maint_id_t)i, vd->total_km);
    build_alerts(ds, vd);
    ds->settings = *s;
}

// ---------------------------------------------------------------
// Callbacks da UI
// ---------------------------------------------------------------
typedef struct
{
    vehicle_data_t *vd;
    app_settings_t *s;
    touch_cal_t *cal;
} app_cb_t;

static void on_register(uint8_t idx, int32_t km,
                        uint8_t day, uint8_t month, uint16_t year, void *ud)
{
    app_cb_t *cb = (app_cb_t *)ud;
    if (idx >= MAINT_COUNT)
        return;
    maint_register(&cb->vd->items[idx], km, day, month, year);
    maint_nvs_save(cb->vd);
    maint_spiffs_log((maint_id_t)idx, km, day, month, year);
}

static void on_settings(const app_settings_t *s, void *ud)
{
    app_cb_t *cb = (app_cb_t *)ud;
    *cb->s = *s;
    settings_save(s);
    // Futuro: aplicar brilho PWM, LEDs, etc.
}

static void on_interval(uint8_t idx, int32_t km, void *ud)
{
    app_cb_t *cb = (app_cb_t *)ud;
    if (idx >= MAINT_COUNT)
        return;
    cb->vd->items[idx].interval_km = km;
    maint_nvs_save(cb->vd);
}

static volatile bool g_do_calibrate = false;

static void on_calibrate(void *ud)
{
    g_do_calibrate = true; // flag; tratado no main loop
}

// ---------------------------------------------------------------
// Task OBD
// ---------------------------------------------------------------
static void obd_task(void *arg)
{
    while (1)
    {
        // g_obd.rpm        = obd_get_rpm();
        // g_obd.speed_kmh  = obd_read_pid(0x0D);
        // g_obd.temp_c     = obd_read_pid(0x05) - 40;
        // g_obd.fuel_pct   = obd_read_pid(0x2F) * 100 / 255;
        vTaskDelay(pdMS_TO_TICKS(OBD_POLL_MS));
    }
}

// ---------------------------------------------------------------
// Task principal
// ---------------------------------------------------------------
static void main_task(void *arg)
{
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    esp_vfs_spiffs_conf_t sc = {
        .base_path = "/spiffs",
        .partition_label = NULL,
        .max_files = 5,
        .format_if_mount_failed = true,
    };
    esp_vfs_spiffs_register(&sc);

    display_init();
    touch_init();

    static touch_cal_t cal;
    if (calibration_load(&cal) != ESP_OK || !cal.valid)
        calibration_run(&cal);

    static vehicle_data_t vd;
    if (maint_nvs_load(&vd) != ESP_OK)
        maint_init_defaults(&vd);

    static app_settings_t settings;
    if (settings_load(&settings) != ESP_OK)
        settings_defaults(&settings);

    static ui_dataset_t ds;
    memset(&ds, 0, sizeof(ds));

    // Histórico do SPIFFS
    static char log_buf[512];
    if (maint_spiffs_read_log(log_buf, sizeof(log_buf)) == ESP_OK)
    {
        char *line = strtok(log_buf, "\n");
        while (line && ds.hist_n < UI_MAX_HIST)
        {
            ui_hist_row_t *h = &ds.hist[ds.hist_n];
            if (sscanf(line, "%11[^;];%19[^;];%9s", h->date, h->item, h->km) == 3)
                ds.hist_n++;
            line = strtok(NULL, "\n");
        }
    }

    static app_cb_t cb = {0};
    cb.vd = &vd;
    cb.s = &settings;
    cb.cal = &cal;

    update_dataset(&ds, &vd, &settings);

    static ui_ctx_t ctx;
    ui_init(&ctx, &ds, on_register, on_settings, on_interval, on_calibrate, &cb);

    xTaskCreate(obd_task, "obd_task", 4096, NULL, 4, NULL);

    bool was_pressed = false;
    uint32_t last_update = 0;

    while (1)
    {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

        // Calibração via flag (bloqueante, mas só quando solicitado)
        if (g_do_calibrate)
        {
            g_do_calibrate = false;
            calibration_run(&cal);
            ctx.redraw = UI_REDRAW_FULL; // redesenha após calibrar
        }

        if (now - last_update >= OBD_POLL_MS)
        {
            vd.total_km = g_obd.km_total;
            update_dataset(&ds, &vd, &settings);
            ui_update_obd(&ctx);
            last_update = now;
        }

        ui_draw(&ctx);

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