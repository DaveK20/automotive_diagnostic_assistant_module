#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "components/display/display.h"
#include "components/display/ui/ui_data.h"

// ---------------------------------------------------------------
// Telas
// ---------------------------------------------------------------
typedef enum
{
    SCREEN_HOME = 0,
    SCREEN_STATUS,
    SCREEN_MAINTENANCE,
    SCREEN_HISTORY,     // contém aba HIST e aba ALERTAS
    SCREEN_SETTINGS,    // lista de categorias
    SCREEN_REG_FORM,    // formulário de registro (sem nav)
    SCREEN_SET_GENERAL, // Configurações > Geral
    SCREEN_SET_VEHICLE, // Configurações > Veículo/Manutenção
    SCREEN_SET_ALERTS,  // Configurações > Alertas
    SCREEN_SET_LEDS,    // Configurações > LEDs
} ui_screen_t;

// ---------------------------------------------------------------
// Layout base (320×240)
// ---------------------------------------------------------------
#define UI_BAR_H 26
#define UI_NAV_H 30
#define UI_NAV_BTN_W (DISPLAY_WIDTH / 4)
#define UI_NAV_Y (DISPLAY_HEIGHT - UI_NAV_H)             // 210
#define UI_CONT_Y UI_BAR_H                               // 26
#define UI_CONT_H (DISPLAY_HEIGHT - UI_BAR_H - UI_NAV_H) // 184

// ---------------------------------------------------------------
// Toque
// ---------------------------------------------------------------
typedef struct
{
    uint16_t x, y, w, h;
} ui_region_t;

static inline bool ui_hit(const ui_region_t *r, int16_t px, int16_t py)
{
    return px >= r->x && px < (int16_t)(r->x + r->w) &&
           py >= r->y && py < (int16_t)(r->y + r->h);
}

// Macro auxiliar para uso no handler — evita structs temporárias
#define TAPPED(x0, y0, w0, h0)                             \
    (px >= (int16_t)(x0) && px < (int16_t)((x0) + (w0)) && \
     py >= (int16_t)(y0) && py < (int16_t)((y0) + (h0)))

// Home
#define HOME_BTN_W 156
#define HOME_BTN_H 100
static const ui_region_t HOME_BTN[4] = {
    {2, 30, HOME_BTN_W, HOME_BTN_H},
    {162, 30, HOME_BTN_W, HOME_BTN_H},
    {2, 134, HOME_BTN_W, HOME_BTN_H},
    {162, 134, HOME_BTN_W, HOME_BTN_H},
};

// Nav inferior (4 abas)
static const ui_region_t UI_NAV[4] = {
    {0, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H},
    {80, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H},
    {160, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H},
    {240, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H},
};

// Histórico — abas internas (abaixo da barra de título)
#define HIST_TAB_H 20
#define HIST_CONT_Y (UI_BAR_H + HIST_TAB_H)                   // 46
#define HIST_CONT_H (DISPLAY_HEIGHT - HIST_CONT_Y - UI_NAV_H) // 164
static const ui_region_t HIST_TAB[2] = {
    {0, UI_BAR_H, 160, HIST_TAB_H},
    {160, UI_BAR_H, 160, HIST_TAB_H},
};

// Configurações — categorias (lista)
static const ui_region_t SET_CAT[4] = {
    {4, 30, DISPLAY_WIDTH - 8, 40},
    {4, 74, DISPLAY_WIDTH - 8, 40},
    {4, 118, DISPLAY_WIDTH - 8, 40},
    {4, 162, DISPLAY_WIDTH - 8, 40},
};

// Botão Voltar (top bar, lado esquerdo)
static const ui_region_t BACK_REGION = {0, 0, 70, UI_BAR_H};

// Manutenção — lista + detalhe
#define MAINT_LIST_W 134
#define MAINT_ROW_H 24
#define MAINT_ROW_X 2
#define MAINT_ROW_W (MAINT_LIST_W - 4)
#define MAINT_ROW_Y(i) (UI_CONT_Y + 2 + (i) * (MAINT_ROW_H + 2))
static const ui_region_t MAINT_REG_BTN = {140, 186, 176, 24};

// Configurações > Veículo — linhas e área de edição
#define VEH_ROW_H 20
#define VEH_ROW_Y(i) (UI_CONT_Y + 2 + (i) * (VEH_ROW_H + 2))
#define VEH_EDIT_Y (UI_CONT_Y + 2 + UI_MAX_MAINT * (VEH_ROW_H + 2))
// VEH_EDIT_Y = 26+2+7*22 = 182, h=26, nav em 210 ✓

// Configurações sub-telas — layout das linhas
#define SET_ROW_Y(i) (UI_CONT_Y + 6 + (i) * 34) // 32, 66, 100, 134, 168
#define SET_BTN_Y(i) (SET_ROW_Y(i) + 4)
#define SET_BTN_H 24
#define SET_MINUS_X 102
#define SET_MINUS_W 28
#define SET_VAL_X 132
#define SET_VAL_W 74
#define SET_PLUS_X 208
#define SET_PLUS_W 28

// Formulário de registro
#define FORM_KM_Y 64
#define FORM_KM_H 28
static const ui_region_t FORM_KM_BTN[5] = {
    {2, FORM_KM_Y, 44, FORM_KM_H},
    {48, FORM_KM_Y, 40, FORM_KM_H},
    {90, FORM_KM_Y, 88, FORM_KM_H},
    {180, FORM_KM_Y, 40, FORM_KM_H},
    {222, FORM_KM_Y, 44, FORM_KM_H},
};
#define FORM_DATE_Y 112
#define FORM_DATE_H 28
static const ui_region_t FORM_DAY_BTN[3] = {
    {2, FORM_DATE_Y, 30, FORM_DATE_H},
    {34, FORM_DATE_Y, 30, FORM_DATE_H},
    {66, FORM_DATE_Y, 30, FORM_DATE_H},
};
static const ui_region_t FORM_MON_BTN[3] = {
    {104, FORM_DATE_Y, 30, FORM_DATE_H},
    {136, FORM_DATE_Y, 30, FORM_DATE_H},
    {168, FORM_DATE_Y, 30, FORM_DATE_H},
};
static const ui_region_t FORM_YR_BTN[3] = {
    {204, FORM_DATE_Y, 30, FORM_DATE_H},
    {236, FORM_DATE_Y, 48, FORM_DATE_H},
    {286, FORM_DATE_Y, 30, FORM_DATE_H},
};
static const ui_region_t FORM_CONFIRM = {4, 154, 152, 30};
static const ui_region_t FORM_CANCEL = {162, 154, 152, 30};

// ---------------------------------------------------------------
// Flags de redesenho
// ---------------------------------------------------------------
#define UI_REDRAW_NONE 0x00
#define UI_REDRAW_DATA 0x01
#define UI_REDRAW_FULL 0x02
#define UI_REDRAW_FORM 0x04 // só valores do formulário

// ---------------------------------------------------------------
// Contexto
// ---------------------------------------------------------------
typedef struct
{
    ui_screen_t screen;
    uint8_t maint_sel;
    uint8_t hist_tab;    // 0=historico, 1=alertas
    uint8_t set_veh_sel; // item selecionado em SET_VEHICLE
    ui_dataset_t *data;
    uint8_t redraw;

    // Cópia editável das configurações
    app_settings_t settings;

    // Cache OBD
    uint16_t last_rpm, last_spd;
    uint8_t last_tmp, last_fuel;

    // Formulário de registro
    int32_t form_km;
    uint8_t form_day, form_month;
    uint16_t form_year;

    // Callbacks — a UI nunca toca em storage diretamente
    void (*on_register)(uint8_t idx, int32_t km,
                        uint8_t day, uint8_t month, uint16_t year,
                        void *ud);
    void (*on_settings)(const app_settings_t *s, void *ud);
    void (*on_interval)(uint8_t idx, int32_t km, void *ud);
    void (*on_calibrate)(void *ud);
    void *userdata;
} ui_ctx_t;

// ---------------------------------------------------------------
// API pública
// ---------------------------------------------------------------
void ui_init(ui_ctx_t *ctx, ui_dataset_t *data,
             void (*on_register)(uint8_t, int32_t, uint8_t, uint8_t, uint16_t, void *),
             void (*on_settings)(const app_settings_t *, void *),
             void (*on_interval)(uint8_t, int32_t, void *),
             void (*on_calibrate)(void *),
             void *userdata);

bool ui_handle_touch(ui_ctx_t *ctx, int16_t px, int16_t py);
void ui_update_obd(ui_ctx_t *ctx);
void ui_draw(ui_ctx_t *ctx);

#endif // UI_H