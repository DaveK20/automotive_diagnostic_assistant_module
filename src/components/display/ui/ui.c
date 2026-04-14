#include "ui.h"
#include "components/display/display.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ================================================================
// Paleta militar
// ================================================================
#define C_BG RGB(42, 46, 22)
#define C_PANEL RGB(62, 68, 32)
#define C_PANEL_D RGB(32, 35, 16)
#define C_BORDER RGB(88, 96, 42)
#define C_HEADER RGB(28, 31, 14)
#define C_SEL RGB(80, 88, 40)
#define C_TEXT COLOR_WHITE
#define C_DIM RGB(168, 178, 96)
#define C_OK RGB(45, 185, 45)
#define C_WARN RGB(220, 140, 0)
#define C_DUE RGB(210, 30, 30)
#define C_CRIT_BG RGB(48, 8, 8)
#define C_WARN_BG RGB(48, 30, 5)
#define C_NO_REC_BG RGB(40, 38, 8)
#define C_NAV_BG RGB(22, 25, 11)
#define C_NAV_ACT RGB(72, 80, 36)
#define C_GAUGE_DIM RGB(28, 32, 14)
#define C_BTN RGB(52, 58, 26)
#define C_BTN_PRESS RGB(90, 100, 42)

// ================================================================
// Helpers de primitivas
// ================================================================
static void draw_icon_car(uint16_t ix, uint16_t iy, uint16_t c)
{
    display_draw_rect(ix + 4, iy + 1, 14, 5, c);
    display_draw_rect(ix + 1, iy + 5, 20, 7, c);
    display_draw_rect(ix + 2, iy + 11, 6, 3, c);
    display_draw_rect(ix + 14, iy + 11, 6, 3, c);
    display_draw_rect(ix + 5, iy + 2, 4, 3, C_PANEL_D);
    display_draw_rect(ix + 13, iy + 2, 4, 3, C_PANEL_D);
}
static void draw_icon_wrench(uint16_t ix, uint16_t iy, uint16_t c)
{
    display_draw_line(ix + 4, iy + 16, ix + 16, iy + 4, c);
    display_draw_line(ix + 5, iy + 16, ix + 17, iy + 4, c);
    display_draw_line(ix + 4, iy + 15, ix + 16, iy + 3, c);
    display_draw_rect(ix + 13, iy + 1, 6, 6, c);
    display_draw_rect(ix + 14, iy + 2, 4, 4, C_PANEL_D);
    display_draw_rect(ix + 1, iy + 14, 5, 5, c);
    display_draw_rect(ix + 2, iy + 15, 3, 3, C_PANEL_D);
}
static void draw_icon_alert_tri(uint16_t ix, uint16_t iy, uint16_t c)
{
    display_draw_line(ix + 10, iy, ix + 20, iy + 17, c);
    display_draw_line(ix + 20, iy + 17, ix, iy + 17, c);
    display_draw_line(ix, iy + 17, ix + 10, iy, c);
    display_draw_rect(ix + 9, iy + 6, 3, 6, C_PANEL_D);
    display_draw_rect(ix + 9, iy + 14, 3, 2, C_PANEL_D);
}
static void draw_icon_clock(uint16_t ix, uint16_t iy, uint16_t c)
{
    display_draw_rect(ix + 4, iy + 1, 12, 2, c);
    display_draw_rect(ix + 2, iy + 3, 16, 2, c);
    display_draw_rect(ix + 1, iy + 5, 18, 10, c);
    display_draw_rect(ix + 2, iy + 15, 16, 2, c);
    display_draw_rect(ix + 4, iy + 17, 12, 2, c);
    display_draw_rect(ix + 3, iy + 3, 14, 13, C_PANEL_D);
    display_draw_line(ix + 10, iy + 5, ix + 10, iy + 9, c);
    display_draw_line(ix + 10, iy + 9, ix + 14, iy + 9, c);
}

// ================================================================
// Componentes reutilizáveis
// ================================================================
static void draw_top_bar(const char *title, const char *right, uint16_t rc)
{
    display_draw_rect(0, 0, DISPLAY_WIDTH, UI_BAR_H, C_HEADER);
    display_draw_rect(0, UI_BAR_H - 1, DISPLAY_WIDTH, 1, C_BORDER);
    display_draw_string(6, 8, title, C_TEXT, C_HEADER, 1);
    if (right)
    {
        uint16_t rx = DISPLAY_WIDTH - strlen(right) * 6 - 4;
        display_draw_string(rx, 8, right, rc, C_HEADER, 1);
    }
}

static void draw_nav(ui_screen_t active)
{
    static const char *lbl[4] = {"STATUS", "MANUT.", "ALERT.", "HIST."};
    static const ui_screen_t scr[4] = {SCREEN_STATUS, SCREEN_MAINTENANCE,
                                       SCREEN_ALERTS, SCREEN_HISTORY};
    display_draw_rect(0, UI_NAV_Y, DISPLAY_WIDTH, UI_NAV_H, C_NAV_BG);
    display_draw_rect(0, UI_NAV_Y, DISPLAY_WIDTH, 1, C_BORDER);
    for (int i = 0; i < 4; i++)
    {
        uint16_t tx = i * UI_NAV_BTN_W;
        bool act = (active == scr[i]);
        uint16_t bg = act ? C_NAV_ACT : C_NAV_BG;
        display_draw_rect(tx, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H, bg);
        if (act)
            display_draw_rect(tx, UI_NAV_Y, UI_NAV_BTN_W, 2, C_OK);
        uint16_t lx = tx + (UI_NAV_BTN_W - strlen(lbl[i]) * 6) / 2;
        display_draw_string(lx, UI_NAV_Y + 10, lbl[i], act ? C_TEXT : C_DIM, bg, 1);
    }
    for (int i = 1; i < 4; i++)
        display_draw_rect(i * UI_NAV_BTN_W, UI_NAV_Y + 2, 1, UI_NAV_H - 4, C_BORDER);
}

// Badge colorido: OK / WARN / DUE / NO_REC
static uint16_t maint_fg(ui_maint_status_t s)
{
    if (s == UI_MAINT_DUE)
        return C_DUE;
    if (s == UI_MAINT_WARN)
        return C_WARN;
    if (s == UI_MAINT_NO_RECORD)
        return C_WARN;
    return C_OK;
}
static uint16_t maint_bg(ui_maint_status_t s)
{
    if (s == UI_MAINT_DUE)
        return C_CRIT_BG;
    if (s == UI_MAINT_WARN)
        return C_WARN_BG;
    if (s == UI_MAINT_NO_RECORD)
        return C_NO_REC_BG;
    return C_PANEL_D;
}
static const char *maint_lbl(ui_maint_status_t s)
{
    if (s == UI_MAINT_DUE)
        return "VENC";
    if (s == UI_MAINT_WARN)
        return "ATEN";
    if (s == UI_MAINT_NO_RECORD)
        return "S/REG";
    return "OK";
}

static void draw_badge(uint16_t x, uint16_t y, ui_maint_status_t s)
{
    uint16_t fg = maint_fg(s), bg = maint_bg(s);
    const char *t = maint_lbl(s);
    uint16_t w = strlen(t) * 6 + 6;
    display_draw_rect(x, y, w, 12, bg);
    display_draw_rect_border(x, y, w, 12, fg, 1);
    display_draw_string(x + 3, y + 2, t, fg, bg, 1);
}

// Ícone de alerta compacto (8×8) ao lado do nome do item na lista
static void draw_warn_dot(uint16_t x, uint16_t y, ui_maint_status_t s)
{
    if (s == UI_MAINT_OK)
        return;
    uint16_t c = (s == UI_MAINT_DUE) ? C_DUE : C_WARN;
    display_draw_rect(x, y, 6, 6, c);
    display_draw_rect(x + 1, y + 1, 4, 4, C_PANEL_D);
    display_draw_rect(x + 2, y + 1, 2, 3, c); // !
    display_draw_rect(x + 2, y + 5, 2, 1, c);
}

static void draw_progress(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                          uint8_t pct, uint16_t color)
{
    if (pct > 100)
        pct = 100;
    display_draw_rect(x, y, w, h, C_PANEL_D);
    if (pct > 0)
        display_draw_rect(x, y, w * pct / 100, h, color);
    display_draw_rect_border(x, y, w, h, C_BORDER, 1);
}

// Botão genérico com label centrado
static void draw_btn(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     const char *lbl, uint16_t bg, uint16_t fg, uint16_t bord)
{
    display_draw_rect(x, y, w, h, bg);
    display_draw_rect_border(x, y, w, h, bord, 1);
    uint16_t lx = x + (w - strlen(lbl) * 6) / 2;
    uint16_t ly = y + (h - 7) / 2;
    display_draw_string(lx, ly, lbl, fg, bg, 1);
}

// ================================================================
// Gauge circular (STATUS)
// ================================================================
static void draw_gauge(int16_t cx, int16_t cy, uint8_t r, uint8_t t, uint8_t pct)
{
    float filled = 210.0f + pct * 2.4f;
    uint8_t ri = r - t;
    for (int deg = 210; deg <= 450; deg += 3)
    {
        float a = deg * 0.01745329f;
        int pos = deg - 210;
        bool act = (deg <= (int)filled);
        uint16_t c;
        if (!act)
            c = C_GAUGE_DIM;
        else if (pos < 80)
            c = C_OK;
        else if (pos < 160)
            c = C_WARN;
        else
            c = C_DUE;
        int16_t x0 = cx + (int16_t)(ri * cosf(a));
        int16_t y0 = cy - (int16_t)(ri * sinf(a));
        int16_t x1 = cx + (int16_t)(r * cosf(a));
        int16_t y1 = cy - (int16_t)(r * sinf(a));
        display_draw_line(x0, y0, x1, y1, c);
        display_draw_line(x0 + 1, y0, x1 + 1, y1, c);
    }
    float na = filled * 0.01745329f;
    int16_t nx = cx + (int16_t)((ri - 2) * cosf(na));
    int16_t ny = cy - (int16_t)((ri - 2) * sinf(na));
    display_draw_line(cx, cy, nx, ny, C_TEXT);
    display_draw_line(cx + 1, cy, nx + 1, ny, C_TEXT);
    display_draw_rect(cx - 2, cy - 2, 4, 4, C_DIM);
}

// ================================================================
// SCREEN_HOME
// ================================================================
static void draw_home(void)
{
    display_fill(C_BG);
    draw_top_bar("CONSOLE DE COMANDO - ASTRA", NULL, 0);
    struct
    {
        const char *lbl;
        uint16_t ic;
    } b[4] = {
        {"STATUS DO VEICULO", C_OK},
        {"MANUTENCAO", C_WARN},
        {"ALERTAS", C_DUE},
        {"HISTORICO", C_DIM},
    };
    for (int i = 0; i < 4; i++)
    {
        const ui_region_t *r = &HOME_BTN[i];
        display_draw_rect(r->x, r->y, r->w, r->h, C_PANEL);
        display_draw_rect_border(r->x, r->y, r->w, r->h, C_BORDER, 1);
        display_draw_rect(r->x + 1, r->y + 1, r->w - 2, 3, b[i].ic);
        uint16_t ix = r->x + (r->w - 22) / 2, iy = r->y + 18;
        switch (i)
        {
        case 0:
            draw_icon_car(ix, iy, b[i].ic);
            break;
        case 1:
            draw_icon_wrench(ix, iy, b[i].ic);
            break;
        case 2:
            draw_icon_alert_tri(ix, iy + 2, b[i].ic);
            break;
        case 3:
            draw_icon_clock(ix, iy, b[i].ic);
            break;
        }
        uint16_t lw = strlen(b[i].lbl) * 6;
        display_draw_string(r->x + (r->w - lw) / 2, r->y + r->h - 16, b[i].lbl, b[i].ic, C_PANEL, 1);
    }
}

// ================================================================
// SCREEN_STATUS — frame estático
// ================================================================
static void draw_status_frame(void)
{
    display_fill(C_BG);
    draw_top_bar("VEHICLE STATUS", NULL, 0);
    display_draw_rect(2, UI_CONT_Y + 2, 152, UI_CONT_H - 4, C_PANEL);
    display_draw_rect_border(2, UI_CONT_Y + 2, 152, UI_CONT_H - 4, C_BORDER, 1);
    display_draw_string(8, UI_CONT_Y + 8, "QUILOMETRAGEM", C_DIM, C_PANEL, 1);
    display_draw_string(8, UI_CONT_Y + 34, "TEMP. MOTOR", C_DIM, C_PANEL, 1);
    display_draw_string(8, UI_CONT_Y + 60, "TEMPO DE USO", C_DIM, C_PANEL, 1);
    display_draw_rect(2, UI_CONT_Y + UI_CONT_H - 26, 152, 1, C_BORDER);
    display_draw_rect(158, UI_CONT_Y + 2, 158, UI_CONT_H - 4, C_PANEL_D);
    display_draw_rect_border(158, UI_CONT_Y + 2, 158, UI_CONT_H - 4, C_BORDER, 1);
    display_draw_string(170, UI_CONT_Y + 148, "PRONTO PARA", C_DIM, C_PANEL_D, 1);
    display_draw_string(174, UI_CONT_Y + 160, "SERVICO", C_DIM, C_PANEL_D, 1);
    draw_nav(SCREEN_STATUS);
}

// Apenas valores dinâmicos (partial redraw)
static void draw_status_values(const ui_obd_t *o)
{
    char buf[28];
    // KM
    snprintf(buf, sizeof(buf), "%ld KM", (long)o->total_km);
    display_draw_rect(6, UI_CONT_Y + 18, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 20, buf, C_TEXT, C_PANEL, 1);
    // Temp
    uint16_t tc = (o->temp_c >= 100) ? C_DUE : (o->temp_c >= 90) ? C_WARN
                                                                 : C_OK;
    snprintf(buf, sizeof(buf), "%d C  %s", o->temp_c, o->temp_c >= 100 ? "ALTO" : "NORMAL");
    display_draw_rect(6, UI_CONT_Y + 44, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 46, buf, tc, C_PANEL, 1);
    // Horas
    snprintf(buf, sizeof(buf), "%d HORAS", o->engine_hours);
    display_draw_rect(6, UI_CONT_Y + 70, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 72, buf, C_TEXT, C_PANEL, 1);
    // RPM/VEL strip
    snprintf(buf, sizeof(buf), "RPM:%d  VEL:%dkm/h", o->rpm, o->speed_kmh);
    display_draw_rect(3, UI_CONT_Y + UI_CONT_H - 24, 150, 20, C_PANEL_D);
    display_draw_string(5, UI_CONT_Y + UI_CONT_H - 18, buf, C_DIM, C_PANEL_D, 1);
    // Gauge
    display_draw_rect(165, UI_CONT_Y + 6, 144, UI_CONT_H - 28, C_PANEL_D);
    draw_gauge(237, UI_CONT_Y + 88, 52, 12, o->readiness_pct);
    snprintf(buf, sizeof(buf), "%d%%", o->readiness_pct);
    uint16_t tw = strlen(buf) * 12;
    display_draw_string(237 - tw / 2, UI_CONT_Y + 78, buf, C_TEXT, C_PANEL_D, 2);
}

// ================================================================
// SCREEN_MAINTENANCE — lista + detalhe
// ================================================================
static void draw_maint_list(const ui_dataset_t *d, uint8_t sel)
{
    display_draw_rect(0, UI_CONT_Y, MAINT_LIST_W + 2, UI_CONT_H, C_PANEL_D);
    display_draw_rect(MAINT_LIST_W + 1, UI_CONT_Y, 1, UI_CONT_H, C_BORDER);
    for (int i = 0; i < d->maint_n && i < UI_MAX_MAINT; i++)
    {
        const ui_maint_row_t *m = &d->maint[i];
        uint16_t ry = MAINT_ROW_Y(i);
        if (ry + MAINT_ROW_H > UI_NAV_Y)
            break;
        bool act = (i == sel);
        uint16_t bg = act ? C_SEL : maint_bg(m->status);
        uint16_t bord = maint_fg(m->status);
        display_draw_rect(MAINT_ROW_X, ry, MAINT_ROW_W, MAINT_ROW_H, bg);
        display_draw_rect_border(MAINT_ROW_X, ry, MAINT_ROW_W, MAINT_ROW_H, bord, 1);
        // Barra lateral colorida (3px)
        display_draw_rect(MAINT_ROW_X, ry, 3, MAINT_ROW_H, bord);
        // Nome
        char name[12];
        strncpy(name, m->name, 11);
        name[11] = '\0';
        display_draw_string(MAINT_ROW_X + 6, ry + 4, name, C_TEXT, bg, 1);
        // Sub-status (km restantes ou sem registro)
        char sub[18];
        if (!m->valid)
            snprintf(sub, sizeof(sub), "sem registro");
        else if (m->km_remaining <= 0)
            snprintf(sub, sizeof(sub), "-%ld km", (long)(-m->km_remaining));
        else
            snprintf(sub, sizeof(sub), "+%ld km", (long)m->km_remaining);
        display_draw_string(MAINT_ROW_X + 6, ry + 14, sub, maint_fg(m->status), bg, 1);
        // Ícone de alerta (canto direito)
        if (m->status != UI_MAINT_OK)
            draw_warn_dot(MAINT_ROW_W - 10, ry + 9, m->status);
    }
}

static void draw_maint_detail(const ui_dataset_t *d, uint8_t sel)
{
    if (sel >= d->maint_n)
        return;
    const ui_maint_row_t *m = &d->maint[sel];
    uint16_t dx = MAINT_LIST_W + 4, dw = DISPLAY_WIDTH - dx - 2;
    uint16_t fg = maint_fg(m->status);

    display_draw_rect(dx, UI_CONT_Y, dw, UI_CONT_H, C_PANEL);
    display_draw_rect_border(dx, UI_CONT_Y, dw, UI_CONT_H, C_BORDER, 1);
    // Título
    uint16_t ty = UI_CONT_Y + 4;
    display_draw_rect(dx, ty, dw, 14, C_SEL);
    uint16_t lx = dx + (dw - strlen(m->name) * 6) / 2;
    display_draw_string(lx, ty + 3, m->name, C_TEXT, C_SEL, 1);

    char buf[28];
    uint16_t vy = ty + 20;
#define DROW(key, val, vc)                                       \
    {                                                            \
        display_draw_string(dx + 4, vy, key, C_DIM, C_PANEL, 1); \
        display_draw_string(dx + 92, vy, val, vc, C_PANEL, 1);   \
        vy += 13;                                                \
    }
    if (m->valid)
        snprintf(buf, sizeof(buf), "%s", m->last_date);
    else
        snprintf(buf, sizeof(buf), "---");
    DROW("ULTIMA:", buf, C_TEXT);
    snprintf(buf, sizeof(buf), "%ld KM", (long)m->last_km);
    DROW("KM TROCA:", buf, C_TEXT);
    snprintf(buf, sizeof(buf), "%ld KM", (long)m->interval_km);
    DROW("INTERVALO:", buf, C_TEXT);
    snprintf(buf, sizeof(buf), "%ld KM", (long)m->next_km);
    DROW("PROXIMA:", buf, fg);
    if (!m->valid)
        snprintf(buf, sizeof(buf), "SEM REGISTRO");
    else if (m->km_remaining <= 0)
        snprintf(buf, sizeof(buf), "VENC. %ldKM", (long)(-m->km_remaining));
    else
        snprintf(buf, sizeof(buf), "FALT. %ldKM", (long)m->km_remaining);
    DROW("STATUS:", buf, fg);
#undef DROW
    // Barra de progresso
    draw_progress(dx + 4, vy, dw - 8, 6, m->progress_pct, fg);
    draw_badge(dx + 4, vy + 10, m->status);

    // Botão REALIZAR TROCA
    draw_btn(MAINT_REG_BTN.x, MAINT_REG_BTN.y,
             MAINT_REG_BTN.w, MAINT_REG_BTN.h,
             "REALIZAR TROCA", C_PANEL_D, C_OK, C_OK);
}

static void draw_maintenance(const ui_dataset_t *d, uint8_t sel)
{
    display_fill(C_BG);
    uint8_t n_bad = 0;
    for (int i = 0; i < d->maint_n; i++)
        if (d->maint[i].status != UI_MAINT_OK)
            n_bad++;
    char rstr[12] = "OK";
    uint16_t rc = C_OK;
    if (n_bad)
    {
        snprintf(rstr, sizeof(rstr), "%d ALERT.", n_bad);
        rc = C_WARN;
    }
    draw_top_bar("MANUTENCAO - REGISTRO DE SERVICOS", rstr, rc);
    draw_maint_list(d, sel);
    draw_maint_detail(d, sel);
    draw_nav(SCREEN_MAINTENANCE);
}

// ================================================================
// SCREEN_REG_FORM — formulário KM + data
// ================================================================

// Redraw apenas os campos de valor (partial redraw no form)
static void draw_form_values(const ui_ctx_t *ctx)
{
    char buf[12];
    // Valor KM
    snprintf(buf, sizeof(buf), "%ld KM", (long)ctx->form_km);
    const ui_region_t *kv = &FORM_KM_BTN[2]; // posição do display de valor
    display_draw_rect(kv->x + 1, kv->y + 1, kv->w - 2, kv->h - 2, C_PANEL_D);
    uint16_t lx = kv->x + (kv->w - strlen(buf) * 6) / 2;
    display_draw_string(lx, kv->y + 10, buf, C_OK, C_PANEL_D, 1);

    // Dia
    snprintf(buf, sizeof(buf), "%02d", ctx->form_day);
    display_draw_rect(FORM_DAY_BTN[1].x + 1, FORM_DAY_BTN[1].y + 1,
                      FORM_DAY_BTN[1].w - 2, FORM_DAY_BTN[1].h - 2, C_PANEL_D);
    display_draw_string(FORM_DAY_BTN[1].x + 9, FORM_DAY_BTN[1].y + 10,
                        buf, C_TEXT, C_PANEL_D, 1);

    // Mês
    snprintf(buf, sizeof(buf), "%02d", ctx->form_month);
    display_draw_rect(FORM_MON_BTN[1].x + 1, FORM_MON_BTN[1].y + 1,
                      FORM_MON_BTN[1].w - 2, FORM_MON_BTN[1].h - 2, C_PANEL_D);
    display_draw_string(FORM_MON_BTN[1].x + 9, FORM_MON_BTN[1].y + 10,
                        buf, C_TEXT, C_PANEL_D, 1);

    // Ano
    snprintf(buf, sizeof(buf), "%04d", ctx->form_year);
    display_draw_rect(FORM_YR_BTN[1].x + 1, FORM_YR_BTN[1].y + 1,
                      FORM_YR_BTN[1].w - 2, FORM_YR_BTN[1].h - 2, C_PANEL_D);
    display_draw_string(FORM_YR_BTN[1].x + 6, FORM_YR_BTN[1].y + 10,
                        buf, C_TEXT, C_PANEL_D, 1);
}

static void draw_reg_form(const ui_ctx_t *ctx)
{
    display_fill(C_BG);
    // Título com nome do item selecionado
    const ui_maint_row_t *m = &ctx->data->maint[ctx->maint_sel];
    char title[40];
    snprintf(title, sizeof(title), "REGISTRAR: %s", m->name);
    draw_top_bar(title, NULL, 0);

    // --- Seção KM ---
    display_draw_string(4, UI_CONT_Y + 8, "QUILOMETRAGEM:", C_DIM, C_BG, 1);
    // Botões −10k e −1k
    draw_btn(FORM_KM_BTN[0].x, FORM_KM_BTN[0].y, FORM_KM_BTN[0].w, FORM_KM_BTN[0].h,
             "-10k", C_BTN, C_DIM, C_BORDER);
    draw_btn(FORM_KM_BTN[1].x, FORM_KM_BTN[1].y, FORM_KM_BTN[1].w, FORM_KM_BTN[1].h,
             "-1k", C_BTN, C_DIM, C_BORDER);
    // Display de valor (fundo)
    display_draw_rect(FORM_KM_BTN[2].x, FORM_KM_BTN[2].y,
                      FORM_KM_BTN[2].w, FORM_KM_BTN[2].h, C_PANEL_D);
    display_draw_rect_border(FORM_KM_BTN[2].x, FORM_KM_BTN[2].y,
                             FORM_KM_BTN[2].w, FORM_KM_BTN[2].h, C_OK, 1);
    // Botões +1k e +10k
    draw_btn(FORM_KM_BTN[3].x, FORM_KM_BTN[3].y, FORM_KM_BTN[3].w, FORM_KM_BTN[3].h,
             "+1k", C_BTN, C_DIM, C_BORDER);
    draw_btn(FORM_KM_BTN[4].x, FORM_KM_BTN[4].y, FORM_KM_BTN[4].w, FORM_KM_BTN[4].h,
             "+10k", C_BTN, C_DIM, C_BORDER);

    // --- Seção DATA ---
    display_draw_string(4, UI_CONT_Y + 60, "DATA (DD / MM / AAAA):", C_DIM, C_BG, 1);
    // Labels dos grupos
    display_draw_string(FORM_DAY_BTN[0].x + 2, FORM_DATE_Y - 14, "DIA", C_DIM, C_BG, 1);
    display_draw_string(FORM_MON_BTN[0].x + 2, FORM_DATE_Y - 14, "MES", C_DIM, C_BG, 1);
    display_draw_string(FORM_YR_BTN[0].x + 4, FORM_DATE_Y - 14, "ANO", C_DIM, C_BG, 1);
    // Divisores de grupo
    display_draw_rect(96, FORM_DATE_Y - 2, 1, FORM_DATE_H + 4, C_BORDER);
    display_draw_rect(198, FORM_DATE_Y - 2, 1, FORM_DATE_H + 4, C_BORDER);
    // Botões dia
    draw_btn(FORM_DAY_BTN[0].x, FORM_DAY_BTN[0].y,
             FORM_DAY_BTN[0].w, FORM_DAY_BTN[0].h, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_DAY_BTN[1].x, FORM_DAY_BTN[1].y,
                      FORM_DAY_BTN[1].w, FORM_DAY_BTN[1].h, C_PANEL_D);
    display_draw_rect_border(FORM_DAY_BTN[1].x, FORM_DAY_BTN[1].y,
                             FORM_DAY_BTN[1].w, FORM_DAY_BTN[1].h, C_BORDER, 1);
    draw_btn(FORM_DAY_BTN[2].x, FORM_DAY_BTN[2].y,
             FORM_DAY_BTN[2].w, FORM_DAY_BTN[2].h, "+", C_BTN, C_DIM, C_BORDER);
    // Botões mês
    draw_btn(FORM_MON_BTN[0].x, FORM_MON_BTN[0].y,
             FORM_MON_BTN[0].w, FORM_MON_BTN[0].h, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_MON_BTN[1].x, FORM_MON_BTN[1].y,
                      FORM_MON_BTN[1].w, FORM_MON_BTN[1].h, C_PANEL_D);
    display_draw_rect_border(FORM_MON_BTN[1].x, FORM_MON_BTN[1].y,
                             FORM_MON_BTN[1].w, FORM_MON_BTN[1].h, C_BORDER, 1);
    draw_btn(FORM_MON_BTN[2].x, FORM_MON_BTN[2].y,
             FORM_MON_BTN[2].w, FORM_MON_BTN[2].h, "+", C_BTN, C_DIM, C_BORDER);
    // Botões ano
    draw_btn(FORM_YR_BTN[0].x, FORM_YR_BTN[0].y,
             FORM_YR_BTN[0].w, FORM_YR_BTN[0].h, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_YR_BTN[1].x, FORM_YR_BTN[1].y,
                      FORM_YR_BTN[1].w, FORM_YR_BTN[1].h, C_PANEL_D);
    display_draw_rect_border(FORM_YR_BTN[1].x, FORM_YR_BTN[1].y,
                             FORM_YR_BTN[1].w, FORM_YR_BTN[1].h, C_BORDER, 1);
    draw_btn(FORM_YR_BTN[2].x, FORM_YR_BTN[2].y,
             FORM_YR_BTN[2].w, FORM_YR_BTN[2].h, "+", C_BTN, C_DIM, C_BORDER);

    // Separador
    display_draw_rect(2, FORM_DATE_Y + FORM_DATE_H + 8, DISPLAY_WIDTH - 4, 1, C_BORDER);

    // Botões CONFIRMAR / CANCELAR
    draw_btn(FORM_CONFIRM.x, FORM_CONFIRM.y, FORM_CONFIRM.w, FORM_CONFIRM.h,
             "CONFIRMAR", C_PANEL_D, C_OK, C_OK);
    draw_btn(FORM_CANCEL.x, FORM_CANCEL.y, FORM_CANCEL.w, FORM_CANCEL.h,
             "CANCELAR", C_PANEL_D, C_DUE, C_DUE);

    // Valores iniciais
    draw_form_values(ctx);
}

// ================================================================
// SCREEN_ALERTS — ordenado (crítico primeiro)
// ================================================================
static void draw_alerts(const ui_dataset_t *d)
{
    display_fill(C_BG);
    uint8_t nc = 0, nw = 0;
    for (int i = 0; i < d->alert_n; i++)
    {
        if (d->alerts[i].level == UI_ALERT_CRITICAL)
            nc++;
        else
            nw++;
    }
    char hdr[20] = "OK";
    uint16_t hc = C_OK;
    if (nc)
    {
        snprintf(hdr, sizeof(hdr), "%d CRIT / %d ATEN", nc, nw);
        hc = C_DUE;
    }
    else if (nw)
    {
        snprintf(hdr, sizeof(hdr), "%d ATENCAO", nw);
        hc = C_WARN;
    }
    draw_top_bar("ALERTAS E LEMBRETES", hdr, hc);

    uint16_t y = UI_CONT_Y + 4;
    if (d->alert_n == 0)
    {
        display_draw_string(10, y + 40, "Nenhum alerta ativo.", C_DIM, C_BG, 1);
        draw_nav(SCREEN_ALERTS);
        return;
    }
    // Dois passes: críticos primeiro, depois warnings
    for (int pass = 0; pass < 2; pass++)
    {
        ui_alert_level_t target = (pass == 0) ? UI_ALERT_CRITICAL : UI_ALERT_WARNING;
        for (int i = 0; i < d->alert_n; i++)
        {
            if (d->alerts[i].level != target)
                continue;
            if (y + 28 > UI_NAV_Y)
                break;
            const ui_alert_row_t *a = &d->alerts[i];
            uint16_t bg = (target == UI_ALERT_CRITICAL) ? C_CRIT_BG : C_WARN_BG;
            uint16_t bord = (target == UI_ALERT_CRITICAL) ? C_DUE : C_WARN;
            uint16_t fc = bord;
            display_draw_rect(4, y, DISPLAY_WIDTH - 8, 26, bg);
            display_draw_rect_border(4, y, DISPLAY_WIDTH - 8, 26, bord, 1);
            display_draw_rect(4, y, 5, 26, bord); // barra lateral grossa
            draw_icon_alert_tri(12, y + 4, fc);
            display_draw_string(38, y + 9, a->text, fc, bg, 1);
            y += 30;
        }
    }
    draw_nav(SCREEN_ALERTS);
}

// ================================================================
// SCREEN_HISTORY
// ================================================================
static void draw_history(const ui_dataset_t *d)
{
    display_fill(C_BG);
    draw_top_bar("HISTORICO DE MANUTENCAO", NULL, 0);
    uint16_t hy = UI_CONT_Y + 2;
    // Cabeçalho
    display_draw_rect(2, hy, DISPLAY_WIDTH - 4, 16, C_PANEL_D);
    display_draw_rect_border(2, hy, DISPLAY_WIDTH - 4, 16, C_BORDER, 1);
    display_draw_string(6, hy + 4, "DATA", C_DIM, C_PANEL_D, 1);
    display_draw_string(76, hy + 4, "ITEM", C_DIM, C_PANEL_D, 1);
    display_draw_string(196, hy + 4, "KM", C_DIM, C_PANEL_D, 1);
    display_draw_string(262, hy + 4, "STATUS", C_DIM, C_PANEL_D, 1);
    for (int i = 1; i < 4; i++)
    {
        static const uint16_t cx[3] = {72, 192, 258};
        display_draw_rect(cx[i - 1], hy, 1, 16, C_BORDER);
    }
    hy += 18;
    if (d->hist_n == 0)
    {
        display_draw_string(10, hy + 20, "Nenhum registro salvo.", C_DIM, C_BG, 1);
        draw_nav(SCREEN_HISTORY);
        return;
    }
    for (int i = 0; i < d->hist_n && i < UI_MAX_HIST; i++)
    {
        if (hy + 20 > UI_NAV_Y)
            break;
        const ui_hist_row_t *h = &d->hist[i];
        uint16_t bg = (i & 1) ? C_PANEL : C_PANEL_D;
        display_draw_rect(2, hy, DISPLAY_WIDTH - 4, 18, bg);
        display_draw_rect_border(2, hy, DISPLAY_WIDTH - 4, 18, C_BORDER, 1);
        display_draw_rect(2, hy, 3, 18, C_OK);
        display_draw_string(6, hy + 5, h->date, C_DIM, bg, 1);
        display_draw_string(76, hy + 5, h->item, C_TEXT, bg, 1);
        char kmf[16];
        snprintf(kmf, sizeof(kmf), "%s KM", h->km);
        display_draw_string(196, hy + 5, kmf, C_DIM, bg, 1);
        display_draw_string(266, hy + 5, "OK", C_OK, bg, 1);
        static const uint16_t cx[3] = {72, 192, 258};
        for (int c = 0; c < 3; c++)
            display_draw_rect(cx[c], hy, 1, 18, C_BORDER);
        hy += 20;
    }
    draw_nav(SCREEN_HISTORY);
}

// ================================================================
// API pública
// ================================================================

void ui_init(ui_ctx_t *ctx, ui_dataset_t *data,
             void (*on_register)(uint8_t, int32_t, uint8_t, uint8_t, uint16_t, void *),
             void *userdata)
{
    ctx->screen = SCREEN_HOME;
    ctx->maint_sel = 0;
    ctx->data = data;
    ctx->redraw = UI_REDRAW_FULL;
    ctx->last_rpm = 0xFFFF;
    ctx->last_spd = 0xFFFF;
    ctx->last_tmp = 0xFF;
    ctx->last_fuel = 0xFF;
    ctx->form_km = data->obd.total_km;
    ctx->form_day = 1;
    ctx->form_month = 1;
    ctx->form_year = 2025;
    ctx->on_register = on_register;
    ctx->userdata = userdata;
}

void ui_update_obd(ui_ctx_t *ctx)
{
    if (ctx->screen != SCREEN_STATUS)
        return;
    const ui_obd_t *o = &ctx->data->obd;
    if (o->rpm == ctx->last_rpm && o->speed_kmh == ctx->last_spd &&
        o->temp_c == ctx->last_tmp && o->fuel_pct == ctx->last_fuel)
        return;
    ctx->last_rpm = o->rpm;
    ctx->last_spd = o->speed_kmh;
    ctx->last_tmp = o->temp_c;
    ctx->last_fuel = o->fuel_pct;
    if (!(ctx->redraw & UI_REDRAW_FULL))
        ctx->redraw |= UI_REDRAW_DATA;
}

// Clamp helpers
static inline int32_t clamp32(int32_t v, int32_t lo, int32_t hi)
{
    return v < lo ? lo : (v > hi ? hi : v);
}
static inline uint8_t clamp8(int v, int lo, int hi)
{
    return (uint8_t)(v < lo ? lo : (v > hi ? hi : v));
}
static inline uint16_t clamp16(int v, int lo, int hi)
{
    return (uint16_t)(v < lo ? lo : (v > hi ? hi : v));
}

bool ui_handle_touch(ui_ctx_t *ctx, int16_t px, int16_t py)
{
    bool changed = false;

    // Navegação inferior (todas as telas com nav bar)
    if (ctx->screen != SCREEN_HOME && ctx->screen != SCREEN_REG_FORM)
    {
        static const ui_screen_t nav_dest[4] = {
            SCREEN_STATUS, SCREEN_MAINTENANCE, SCREEN_ALERTS, SCREEN_HISTORY};
        for (int i = 0; i < 4; i++)
        {
            if (ui_hit(&UI_NAV[i], px, py) && ctx->screen != nav_dest[i])
            {
                ctx->screen = nav_dest[i];
                changed = true;
                break;
            }
        }
    }

    if (!changed)
        switch (ctx->screen)
        {

        case SCREEN_HOME:
            for (int i = 0; i < 4; i++)
            {
                if (ui_hit(&HOME_BTN[i], px, py))
                {
                    ctx->screen = (ui_screen_t)(SCREEN_STATUS + i);
                    changed = true;
                    break;
                }
            }
            break;

        case SCREEN_MAINTENANCE:
            // Seleção de item na lista
            for (int i = 0; i < ctx->data->maint_n; i++)
            {
                ui_region_t r = {MAINT_ROW_X, MAINT_ROW_Y(i), MAINT_ROW_W, MAINT_ROW_H};
                if (ui_hit(&r, px, py))
                {
                    if (ctx->maint_sel != (uint8_t)i)
                    {
                        ctx->maint_sel = i;
                        changed = true;
                    }
                    break;
                }
            }
            // Botão REALIZAR TROCA → abre formulário
            if (!changed && ui_hit(&MAINT_REG_BTN, px, py))
            {
                // Pré-carrega KM atual no formulário
                ctx->form_km = ctx->data->obd.total_km;
                ctx->screen = SCREEN_REG_FORM;
                changed = true;
            }
            break;

        case SCREEN_REG_FORM:
        {
            bool form_changed = false;

            // KM: −10k −1k (val) +1k +10k
            if (ui_hit(&FORM_KM_BTN[0], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km - 10000, 0, 999999);
                form_changed = true;
            }
            else if (ui_hit(&FORM_KM_BTN[1], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km - 1000, 0, 999999);
                form_changed = true;
            }
            else if (ui_hit(&FORM_KM_BTN[3], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km + 1000, 0, 999999);
                form_changed = true;
            }
            else if (ui_hit(&FORM_KM_BTN[4], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km + 10000, 0, 999999);
                form_changed = true;
            }
            // Dia
            else if (ui_hit(&FORM_DAY_BTN[0], px, py))
            {
                ctx->form_day = clamp8(ctx->form_day - 1, 1, 31);
                form_changed = true;
            }
            else if (ui_hit(&FORM_DAY_BTN[2], px, py))
            {
                ctx->form_day = clamp8(ctx->form_day + 1, 1, 31);
                form_changed = true;
            }
            // Mês
            else if (ui_hit(&FORM_MON_BTN[0], px, py))
            {
                ctx->form_month = clamp8(ctx->form_month - 1, 1, 12);
                form_changed = true;
            }
            else if (ui_hit(&FORM_MON_BTN[2], px, py))
            {
                ctx->form_month = clamp8(ctx->form_month + 1, 1, 12);
                form_changed = true;
            }
            // Ano
            else if (ui_hit(&FORM_YR_BTN[0], px, py))
            {
                ctx->form_year = clamp16(ctx->form_year - 1, 2000, 2099);
                form_changed = true;
            }
            else if (ui_hit(&FORM_YR_BTN[2], px, py))
            {
                ctx->form_year = clamp16(ctx->form_year + 1, 2000, 2099);
                form_changed = true;
            }
            // Confirmar
            else if (ui_hit(&FORM_CONFIRM, px, py))
            {
                if (ctx->on_register)
                    ctx->on_register(ctx->maint_sel,
                                     ctx->form_km,
                                     ctx->form_day, ctx->form_month, ctx->form_year,
                                     ctx->userdata);
                ctx->screen = SCREEN_MAINTENANCE;
                changed = true;
            }
            // Cancelar
            else if (ui_hit(&FORM_CANCEL, px, py))
            {
                ctx->screen = SCREEN_MAINTENANCE;
                changed = true;
            }

            if (form_changed)
            {
                if (!(ctx->redraw & UI_REDRAW_FULL))
                    ctx->redraw |= UI_REDRAW_FORM;
                return true; // não seta FULL, só FORM
            }
            break;
        }

        default:
            break;
        }

    if (changed)
        ctx->redraw = UI_REDRAW_FULL;
    return changed;
}

void ui_draw(ui_ctx_t *ctx)
{
    if (ctx->redraw == UI_REDRAW_NONE)
        return;

    if (ctx->redraw & UI_REDRAW_FULL)
    {
        switch (ctx->screen)
        {
        case SCREEN_HOME:
            draw_home();
            break;
        case SCREEN_STATUS:
            draw_status_frame();
            draw_status_values(&ctx->data->obd);
            break;
        case SCREEN_MAINTENANCE:
            draw_maintenance(ctx->data, ctx->maint_sel);
            break;
        case SCREEN_ALERTS:
            draw_alerts(ctx->data);
            break;
        case SCREEN_HISTORY:
            draw_history(ctx->data);
            break;
        case SCREEN_REG_FORM:
            draw_reg_form(ctx);
            break;
        }
    }
    else if (ctx->redraw & UI_REDRAW_DATA)
    {
        draw_status_values(&ctx->data->obd);
    }
    else if (ctx->redraw & UI_REDRAW_FORM)
    {
        draw_form_values(ctx);
    }

    ctx->redraw = UI_REDRAW_NONE;
}