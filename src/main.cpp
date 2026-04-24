#include <stdio.h>
#include <string.h>
#include <iostream>
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "nvs_flash.h"
#include "esp_timer.h"
#include "driver/gpio.h"
#include "driver/spi_common.h"
#include "driver/sdspi_host.h"
#include "esp_vfs_fat.h"
#include "sdmmc_cmd.h"
#include "components/display/display.h"
#include "components/display/calibration.h"
#include "components/maintenance/maintenance.h"
#include "components/settings/settings.h"
#include "components/display/ui/ui_data.h"
#include "components/display/ui/ui.h"

// ---------------------------------------------------------------
// Pinos SD (SPI) — ajuste conforme seu hardware
// ---------------------------------------------------------------
#define SD_MOSI_PIN 23
#define SD_MISO_PIN 19
#define SD_CLK_PIN 18
#define SD_CS_PIN 5
#define SD_SPI_HOST SPI2_HOST
#define SD_MOUNT_POINT "/sd"

#define OBD_POLL_MS 300
#define TOUCH_POLL_MS 20

// ---------------------------------------------------------------
// Dados OBD (atualizados pela task OBD)
// ---------------------------------------------------------------
static volatile struct
{
    uint16_t rpm, speed_kmh;
    uint8_t temp_c, fuel_pct;
    int32_t km_total;
    uint16_t engine_hours;
} g_obd = {0};

// ---------------------------------------------------------------
// Inicialização do SD via SPI (FATFS)
// ---------------------------------------------------------------
static sdmmc_card_t *sd_card = NULL;

static esp_err_t sd_init(void)
{
    spi_bus_config_t bus = {
        .mosi_io_num = SD_MOSI_PIN,
        .miso_io_num = SD_MISO_PIN,
        .sclk_io_num = SD_CLK_PIN,
        .quadwp_io_num = -1,
        .quadhd_io_num = -1,
        .max_transfer_sz = 4096,
    };
    esp_err_t ret = spi_bus_initialize(SD_SPI_HOST, &bus, SDSPI_DEFAULT_DMA);
    if (ret != ESP_OK)
    {
        printf("[SD] Falha ao inicializar barramento SPI: %s\n", esp_err_to_name(ret));
        return ret;
    }

    sdspi_device_config_t slot = SDSPI_DEVICE_CONFIG_DEFAULT();
    slot.gpio_cs = (gpio_num_t)SD_CS_PIN;
    slot.host_id = SD_SPI_HOST;

    esp_vfs_fat_sdmmc_mount_config_t mount_cfg = {
        .format_if_mount_failed = false,
        .max_files = 5,
        .allocation_unit_size = 16 * 1024,
    };

    sdmmc_host_t host = SDSPI_HOST_DEFAULT();
    host.slot = SD_SPI_HOST;

    ret = esp_vfs_fat_sdspi_mount(SD_MOUNT_POINT, &host, &slot, &mount_cfg, &sd_card);
    if (ret != ESP_OK)
    {
        printf("[SD] Falha ao montar: %s\n", esp_err_to_name(ret));
        spi_bus_free(SD_SPI_HOST);
        return ret;
    }

    printf("[SD] Montado em %s — %.0f MB\n",
           SD_MOUNT_POINT,
           (double)((uint64_t)sd_card->csd.capacity * sd_card->csd.sector_size) / (1024 * 1024));
    return ESP_OK;
}

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
// Preenche ui_maint_row_t
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

    unsigned int day = item->last_day > 99 ? 99 : item->last_day;
    unsigned int mon = item->last_month > 99 ? 99 : item->last_month;
    unsigned int yr = item->last_year > 9999 ? 9999 : item->last_year;
    snprintf(row->last_date, sizeof(row->last_date), "%02u/%02u/%04u", day, mon, yr);
}

// ---------------------------------------------------------------
// Alertas ordenados: DUE → WARN → NO_RECORD
// ---------------------------------------------------------------
static void build_alerts(ui_dataset_t *ds, const vehicle_data_t *vd)
{
    ds->alert_n = 0;
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
// Atualiza dataset (OBD + manutenção + alertas)
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
// Recarrega histórico do SD para o dataset
// ---------------------------------------------------------------
static void reload_history(ui_dataset_t *ds)
{
    static maint_log_entry_t log_buf[UI_MAX_HIST];
    uint8_t n = 0;
    maint_sd_read_log(log_buf, UI_MAX_HIST, &n);
    ui_dataset_populate_hist(ds, log_buf, n);
}

// ---------------------------------------------------------------
// Contexto dos callbacks
// ---------------------------------------------------------------
typedef struct
{
    vehicle_data_t *vd;
    app_settings_t *s;
    touch_cal_t *cal;
    ui_dataset_t *ds;
} app_cb_t;

// ---------------------------------------------------------------
// Callbacks da UI
// ---------------------------------------------------------------
static void on_register(uint8_t idx, int32_t km,
                        uint8_t day, uint8_t month, uint16_t year, void *ud)
{
    app_cb_t *cb = (app_cb_t *)ud;
    if (idx >= MAINT_COUNT)
        return;

    // Atualiza estado em memória
    maint_register(&cb->vd->items[idx], km, day, month, year);

    // Salva estado atual no SD (/sd/maint_data.json)
    maint_sd_save(cb->vd);

    // Acrescenta entrada no histórico (/sd/maint_log.json)
    maint_sd_log_entry((maint_id_t)idx, km, day, month, year);

    // Recarrega histórico na UI
    reload_history(cb->ds);
}

static void on_settings(const app_settings_t *s, void *ud)
{
    app_cb_t *cb = (app_cb_t *)ud;
    *cb->s = *s;
    settings_save(s);
}

static void on_interval(uint8_t idx, int32_t km, void *ud)
{
    app_cb_t *cb = (app_cb_t *)ud;
    if (idx >= MAINT_COUNT)
        return;
    cb->vd->items[idx].interval_km = km;
    maint_sd_save(cb->vd); // persiste o novo intervalo
}

static volatile bool g_do_calibrate = false;
static void on_calibrate(void *ud) { g_do_calibrate = true; }

// ---------------------------------------------------------------
// Task OBD (stub — preencha com suas leituras reais)
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
    // NVS — necessário para settings
    esp_err_t ret = nvs_flash_init();
    if (ret == ESP_ERR_NVS_NO_FREE_PAGES || ret == ESP_ERR_NVS_NEW_VERSION_FOUND)
    {
        nvs_flash_erase();
        nvs_flash_init();
    }

    // SD card
    bool sd_ok = (sd_init() == ESP_OK);
    if (!sd_ok)
        printf("[AVISO] SD indisponivel — dados nao serao persistidos.\n");

    display_init();
    touch_init();

    static touch_cal_t cal;
    if (calibration_load(&cal) != ESP_OK || !cal.valid)
        calibration_run(&cal);

    // Carrega dados do veículo do SD (ou defaults na 1ª vez)
    static vehicle_data_t vd;
    if (!sd_ok || maint_sd_load(&vd) != ESP_OK)
    {
        maint_init_defaults(&vd);
        if (sd_ok)
            maint_sd_save(&vd); // salva defaults no SD
    }

    static app_settings_t settings;
    if (settings_load(&settings) != ESP_OK)
        settings_defaults(&settings);

    static ui_dataset_t ds;
    memset(&ds, 0, sizeof(ds));

    // Carrega histórico do SD
    if (sd_ok)
        reload_history(&ds);

    // Prepara dataset inicial
    update_dataset(&ds, &vd, &settings);

    static app_cb_t cb;
    cb.vd = &vd;
    cb.s = &settings;
    cb.cal = &cal;
    cb.ds = &ds;

    static ui_ctx_t ctx;
    ui_init(&ctx, &ds, on_register, on_settings, on_interval, on_calibrate, &cb);

    xTaskCreate(obd_task, "obd_task", 4096, NULL, 4, NULL);

    bool was_pressed = false;
    uint32_t last_update = 0;

    while (1)
    {
        uint32_t now = (uint32_t)(esp_timer_get_time() / 1000);

        if (g_do_calibrate)
        {
            g_do_calibrate = false;
            calibration_run(&cal);
            ctx.redraw = UI_REDRAW_FULL;
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

extern "C" void app_main(void)
{
    xTaskCreate(main_task, "main_task", 8192, NULL, 5, NULL);
}