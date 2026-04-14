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
    SCREEN_HOME = 0,       // Menu principal (4 botões grandes)
    SCREEN_STATUS,         // OBD em tempo real + gauge
    SCREEN_MAINTENANCE,    // Lista + detalhe lado a lado
    SCREEN_ALERTS,         // Alertas e lembretes empilhados
    SCREEN_HISTORY,        // Tabela de registros do SPIFFS
    SCREEN_CONFIRM_REG,    // Modal confirmação de registro
} ui_screen_t;

// ---------------------------------------------------------------
// Layout base (320×240)
// ---------------------------------------------------------------
#define UI_BAR_H      26                              // Barra de título
#define UI_NAV_H      30                              // Barra de navegação
#define UI_NAV_BTN_W  (DISPLAY_WIDTH / 4)             // 4 abas × 80px
#define UI_NAV_Y      (DISPLAY_HEIGHT - UI_NAV_H)     // y=210
#define UI_CONT_Y     UI_BAR_H                        // y=26
#define UI_CONT_H     (DISPLAY_HEIGHT - UI_BAR_H - UI_NAV_H) // 184px

// ---------------------------------------------------------------
// Região de toque
// ---------------------------------------------------------------
typedef struct { uint16_t x, y, w, h; } ui_region_t;

static inline bool ui_hit(const ui_region_t *r, int16_t px, int16_t py)
{
    return px >= r->x && px < (int16_t)(r->x + r->w) &&
           py >= r->y && py < (int16_t)(r->y + r->h);
}

// Home — 4 botões em grade 2×2
#define HOME_BTN_W   156
#define HOME_BTN_H   100
static const ui_region_t HOME_BTN[4] = {
    {  2, 30, HOME_BTN_W, HOME_BTN_H }, // STATUS
    {162, 30, HOME_BTN_W, HOME_BTN_H }, // MANUTENCAO
    {  2,134, HOME_BTN_W, HOME_BTN_H }, // ALERTAS
    {162,134, HOME_BTN_W, HOME_BTN_H }, // HISTORICO
};

// Navegação inferior (4 abas)
static const ui_region_t UI_NAV[4] = {
    {   0, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
    {  80, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
    { 160, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
    { 240, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H },
};

// Manutenção — lista (esquerda) e detalhe (direita)
#define MAINT_LIST_W   134
#define MAINT_ROW_H    24
#define MAINT_ROW_X    2
#define MAINT_ROW_W    (MAINT_LIST_W - 4)
#define MAINT_ROW_Y(i) (UI_CONT_Y + 2 + (i) * (MAINT_ROW_H + 2))

// Detalhe: botão registrar
static const ui_region_t MAINT_REG_BTN = { 142, 192, 174, 24 };

// Modal de confirmação
static const ui_region_t CONFIRM_YES = {  30, 148, 110, 26 };
static const ui_region_t CONFIRM_NO  = { 180, 148, 110, 26 };

// ---------------------------------------------------------------
// Flags de redesenho
// ---------------------------------------------------------------
#define UI_REDRAW_NONE  0x00
#define UI_REDRAW_DATA  0x01   // Só valores dinâmicos (STATUS)
#define UI_REDRAW_FULL  0x02   // Tela inteira

// ---------------------------------------------------------------
// Contexto — tudo que a state machine precisa
// ---------------------------------------------------------------
typedef struct {
    ui_screen_t   screen;
    uint8_t       maint_sel;     // Item selecionado na lista de manutenção
    ui_dataset_t *data;
    uint8_t       redraw;

    // Cache OBD para detecção de mudança (STATUS partial redraw)
    uint16_t last_rpm, last_spd;
    uint8_t  last_tmp, last_fuel;

    // Callback: chamado quando usuário confirma registro
    // O app decide o que fazer (NVS, SPIFFS, etc.)
    void (*on_register)(uint8_t maint_idx, void *userdata);
    void *userdata;
} ui_ctx_t;

// ---------------------------------------------------------------
// API pública
// ---------------------------------------------------------------
void ui_init(ui_ctx_t *ctx, ui_dataset_t *data,
             void (*on_register)(uint8_t, void *), void *userdata);

bool ui_handle_touch(ui_ctx_t *ctx, int16_t px, int16_t py);

// Atualiza OBD e seta REDRAW_DATA se algum valor mudou
void ui_update_obd(ui_ctx_t *ctx);

// Desenha conforme ctx->redraw
void ui_draw(ui_ctx_t *ctx);

#endif // UI_H