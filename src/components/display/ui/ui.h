#ifndef UI_H
#define UI_H

#include <stdint.h>
#include <stdbool.h>
#include "components/display/display.h"
#include "components/display/ui/ui_data.h"

// ---------------------------------------------------------------
// Telas
// ---------------------------------------------------------------
typedef enum {
    SCREEN_HOME = 0,
    SCREEN_STATUS,
    SCREEN_MAINTENANCE,
    SCREEN_ALERTS,
    SCREEN_HISTORY,
    SCREEN_REG_FORM,    // Formulário: KM + data do serviço
} ui_screen_t;

// ---------------------------------------------------------------
// Layout base (320×240)
// ---------------------------------------------------------------
#define UI_BAR_H     26
#define UI_NAV_H     30
#define UI_NAV_BTN_W (DISPLAY_WIDTH / 4)   // 80px cada aba
#define UI_NAV_Y     (DISPLAY_HEIGHT - UI_NAV_H) // 210
#define UI_CONT_Y    UI_BAR_H              // 26
#define UI_CONT_H    (DISPLAY_HEIGHT - UI_BAR_H - UI_NAV_H) // 184

// ---------------------------------------------------------------
// Região de toque
// ---------------------------------------------------------------
typedef struct { uint16_t x, y, w, h; } ui_region_t;

static inline bool ui_hit(const ui_region_t *r, int16_t px, int16_t py) {
    return px >= r->x && px < (int16_t)(r->x + r->w) &&
           py >= r->y && py < (int16_t)(r->y + r->h);
}

// Home — grade 2×2
#define HOME_BTN_W 156
#define HOME_BTN_H 100
static const ui_region_t HOME_BTN[4] = {
    {  2, 30, HOME_BTN_W, HOME_BTN_H },
    {162, 30, HOME_BTN_W, HOME_BTN_H },
    {  2,134, HOME_BTN_W, HOME_BTN_H },
    {162,134, HOME_BTN_W, HOME_BTN_H },
};

// Navegação inferior
static const ui_region_t UI_NAV[4] = {
    {  0, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
    { 80, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
    {160, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
    {240, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
};

// Manutenção — lista + detalhe lado a lado
#define MAINT_LIST_W  134
#define MAINT_ROW_H   24
#define MAINT_ROW_X   2
#define MAINT_ROW_W   (MAINT_LIST_W - 4)
#define MAINT_ROW_Y(i) (UI_CONT_Y + 2 + (i) * (MAINT_ROW_H + 2))

// Botão "REALIZAR TROCA" no detalhe
static const ui_region_t MAINT_REG_BTN = { 140, 186, 176, 24 };

// ---------------------------------------------------------------
// Formulário de registro (SCREEN_REG_FORM, sem nav bar)
// ---------------------------------------------------------------
// Linha KM (y=64): [-10k][−1k][ valor ][+1k][+10k]
#define FORM_KM_Y    64
#define FORM_KM_H    28
static const ui_region_t FORM_KM_BTN[5] = {
    {  2, FORM_KM_Y, 44, FORM_KM_H }, // -10000
    { 48, FORM_KM_Y, 40, FORM_KM_H }, // -1000
    { 90, FORM_KM_Y, 88, FORM_KM_H }, // valor (display, não é botão ativo)
    {180, FORM_KM_Y, 40, FORM_KM_H }, // +1000
    {222, FORM_KM_Y, 44, FORM_KM_H }, // +10000
};

// Linha DATA (y=112): [D-][DD][D+]  [M-][MM][M+]  [A-][AAAA][A+]
#define FORM_DATE_Y  112
#define FORM_DATE_H  28
static const ui_region_t FORM_DAY_BTN[3] = {
    {  2, FORM_DATE_Y, 30, FORM_DATE_H }, // dia −
    { 34, FORM_DATE_Y, 30, FORM_DATE_H }, // dia valor
    { 66, FORM_DATE_Y, 30, FORM_DATE_H }, // dia +
};
static const ui_region_t FORM_MON_BTN[3] = {
    {104, FORM_DATE_Y, 30, FORM_DATE_H }, // mes −
    {136, FORM_DATE_Y, 30, FORM_DATE_H }, // mes valor
    {168, FORM_DATE_Y, 30, FORM_DATE_H }, // mes +
};
static const ui_region_t FORM_YR_BTN[3] = {
    {204, FORM_DATE_Y, 30, FORM_DATE_H }, // ano −
    {236, FORM_DATE_Y, 48, FORM_DATE_H }, // ano valor
    {286, FORM_DATE_Y, 30, FORM_DATE_H }, // ano +
};

// Botões CONFIRMAR / CANCELAR
static const ui_region_t FORM_CONFIRM = {  4, 154, 152, 30 };
static const ui_region_t FORM_CANCEL  = {162, 154, 152, 30 };

// ---------------------------------------------------------------
// Flags de redesenho
// ---------------------------------------------------------------
#define UI_REDRAW_NONE  0x00
#define UI_REDRAW_DATA  0x01  // Valores OBD (STATUS)
#define UI_REDRAW_FULL  0x02  // Tela inteira
#define UI_REDRAW_FORM  0x04  // Só valores do formulário (+/- pressionado)

// ---------------------------------------------------------------
// Contexto
// ---------------------------------------------------------------
typedef struct {
    ui_screen_t   screen;
    uint8_t       maint_sel;
    ui_dataset_t *data;
    uint8_t       redraw;

    // Cache OBD
    uint16_t last_rpm, last_spd;
    uint8_t  last_tmp, last_fuel;

    // Estado do formulário de registro
    int32_t  form_km;
    uint8_t  form_day, form_month;
    uint16_t form_year;

    // Callback com km e data preenchidos pelo usuário
    void (*on_register)(uint8_t idx, int32_t km,
                        uint8_t day, uint8_t month, uint16_t year,
                        void *userdata);
    void *userdata;
} ui_ctx_t;

// ---------------------------------------------------------------
// API pública
// ---------------------------------------------------------------
void ui_init(ui_ctx_t *ctx, ui_dataset_t *data,
             void (*on_register)(uint8_t, int32_t, uint8_t, uint8_t, uint16_t, void *),
             void *userdata);

bool ui_handle_touch(ui_ctx_t *ctx, int16_t px, int16_t py);
void ui_update_obd(ui_ctx_t *ctx);
void ui_draw(ui_ctx_t *ctx);

#endif // UI_H