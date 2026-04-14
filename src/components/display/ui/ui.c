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

// ================================================================
// Ícones
// ================================================================
static void icon_car(uint16_t x, uint16_t y, uint16_t c)
{
    display_draw_rect(x + 4, y + 1, 14, 5, c);
    display_draw_rect(x + 1, y + 5, 20, 7, c);
    display_draw_rect(x + 2, y + 11, 6, 3, c);
    display_draw_rect(x + 14, y + 11, 6, 3, c);
    display_draw_rect(x + 5, y + 2, 4, 3, C_PANEL_D);
    display_draw_rect(x + 13, y + 2, 4, 3, C_PANEL_D);
}
static void icon_wrench(uint16_t x, uint16_t y, uint16_t c)
{
    display_draw_line(x + 4, y + 16, x + 16, y + 4, c);
    display_draw_line(x + 5, y + 16, x + 17, y + 4, c);
    display_draw_line(x + 4, y + 15, x + 16, y + 3, c);
    display_draw_rect(x + 13, y + 1, 6, 6, c);
    display_draw_rect(x + 14, y + 2, 4, 4, C_PANEL_D);
    display_draw_rect(x + 1, y + 14, 5, 5, c);
    display_draw_rect(x + 2, y + 15, 3, 3, C_PANEL_D);
}
static void icon_tri(uint16_t x, uint16_t y, uint16_t c)
{
    display_draw_line(x + 10, y, x + 20, y + 17, c);
    display_draw_line(x + 20, y + 17, x, y + 17, c);
    display_draw_line(x, y + 17, x + 10, y, c);
    display_draw_rect(x + 9, y + 6, 3, 6, C_PANEL_D);
    display_draw_rect(x + 9, y + 14, 3, 2, C_PANEL_D);
}
static void icon_clock(uint16_t x, uint16_t y, uint16_t c)
{
    display_draw_rect(x + 4, y + 1, 12, 2, c);
    display_draw_rect(x + 2, y + 3, 16, 2, c);
    display_draw_rect(x + 1, y + 5, 18, 10, c);
    display_draw_rect(x + 2, y + 15, 16, 2, c);
    display_draw_rect(x + 4, y + 17, 12, 2, c);
    display_draw_rect(x + 3, y + 3, 14, 13, C_PANEL_D);
    display_draw_line(x + 10, y + 5, x + 10, y + 9, c);
    display_draw_line(x + 10, y + 9, x + 14, y + 9, c);
}
static void icon_gear(uint16_t x, uint16_t y, uint16_t c)
{
    display_draw_rect(x + 8, y + 1, 6, 4, c);
    display_draw_rect(x + 8, y + 17, 6, 4, c);
    display_draw_rect(x + 1, y + 8, 4, 6, c);
    display_draw_rect(x + 17, y + 8, 4, 6, c);
    display_draw_rect(x + 4, y + 4, 4, 4, c);
    display_draw_rect(x + 14, y + 4, 4, 4, c);
    display_draw_rect(x + 4, y + 14, 4, 4, c);
    display_draw_rect(x + 14, y + 14, 4, 4, c);
    display_draw_rect(x + 5, y + 5, 12, 12, c);
    display_draw_rect(x + 8, y + 8, 6, 6, C_PANEL_D);
}
static void icon_led(uint16_t x, uint16_t y, uint16_t c)
{
    display_draw_rect(x + 7, y, 8, 6, c);
    display_draw_rect(x + 4, y + 5, 14, 8, c);
    display_draw_rect(x + 6, y + 12, 10, 3, c);
    display_draw_rect(x + 7, y + 14, 8, 2, c);
    display_draw_rect(x + 4, y + 15, 5, 3, c);
    display_draw_rect(x + 13, y + 15, 5, 3, c);
    display_draw_rect(x + 7, y + 2, 8, 9, C_PANEL_D);
    // raios
    display_draw_line(x + 2, y + 3, x, y + 1, c);
    display_draw_line(x + 20, y + 3, x + 22, y + 1, c);
    display_draw_line(x + 1, y + 8, x - 2, y + 8, c);
    display_draw_line(x + 21, y + 8, x + 24, y + 8, c);
}

// ================================================================
// Componentes base
// ================================================================
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
static uint16_t maint_bg_c(ui_maint_status_t s)
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
    uint16_t fg = maint_fg(s), bg = maint_bg_c(s);
    const char *t = maint_lbl(s);
    uint16_t w = strlen(t) * 6 + 6;
    display_draw_rect(x, y, w, 12, bg);
    display_draw_rect_border(x, y, w, 12, fg, 1);
    display_draw_string(x + 3, y + 2, t, fg, bg, 1);
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

static void draw_btn(uint16_t x, uint16_t y, uint16_t w, uint16_t h,
                     const char *lbl, uint16_t bg, uint16_t fg, uint16_t bord)
{
    display_draw_rect(x, y, w, h, bg);
    display_draw_rect_border(x, y, w, h, bord, 1);
    uint16_t lx = x + (w - strlen(lbl) * 6) / 2;
    uint16_t ly = y + (h - 7) / 2;
    display_draw_string(lx, ly, lbl, fg, bg, 1);
}

// ----------------------------------------------------------------
// Linha de configuração: [Label          ] [-] [  val  ] [+]
// ----------------------------------------------------------------
static void draw_set_inc(uint16_t row_i, const char *label,
                         const char *val, uint16_t val_color)
{
    uint16_t ry = SET_ROW_Y(row_i);
    display_draw_string(SET_MINUS_X - 94, ry + 8, label, C_DIM, C_BG, 1);
    draw_btn(SET_MINUS_X, SET_BTN_Y(row_i), SET_MINUS_W, SET_BTN_H, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(SET_VAL_X, SET_BTN_Y(row_i), SET_VAL_W, SET_BTN_H, C_PANEL_D);
    display_draw_rect_border(SET_VAL_X, SET_BTN_Y(row_i), SET_VAL_W, SET_BTN_H, C_BORDER, 1);
    uint16_t vx = SET_VAL_X + (SET_VAL_W - strlen(val) * 6) / 2;
    display_draw_string(vx, SET_BTN_Y(row_i) + 8, val, val_color, C_PANEL_D, 1);
    draw_btn(SET_PLUS_X, SET_BTN_Y(row_i), SET_PLUS_W, SET_BTN_H, "+", C_BTN, C_DIM, C_BORDER);
}

// Linha toggle 2 opções
static void draw_set_tog2(uint16_t row_i, const char *label,
                          const char *o1, const char *o2, bool first)
{
    uint16_t ry = SET_ROW_Y(row_i);
    display_draw_string(8, ry + 8, label, C_DIM, C_BG, 1);
    draw_btn(102, SET_BTN_Y(row_i), 102, SET_BTN_H, o1,
             first ? C_SEL : C_BTN, first ? C_OK : C_DIM, first ? C_OK : C_BORDER);
    draw_btn(206, SET_BTN_Y(row_i), 110, SET_BTN_H, o2,
             !first ? C_SEL : C_BTN, !first ? C_OK : C_DIM, !first ? C_OK : C_BORDER);
}

// Linha toggle 3 opções
static void draw_set_tog3(uint16_t row_i, const char *label,
                          const char *o[3], uint8_t act)
{
    uint16_t ry = SET_ROW_Y(row_i);
    display_draw_string(8, ry + 8, label, C_DIM, C_BG, 1);
    uint16_t xs[3] = {102, 172, 242};
    for (int i = 0; i < 3; i++)
    {
        bool a = (act == i);
        draw_btn(xs[i], SET_BTN_Y(row_i), 68, SET_BTN_H, o[i],
                 a ? C_SEL : C_BTN, a ? C_OK : C_DIM, a ? C_OK : C_BORDER);
    }
}

// Linha toggle 4 opções (modo LED)
static void draw_set_tog4(uint16_t row_i, const char *label,
                          const char *o[4], uint8_t act)
{
    uint16_t ry = SET_ROW_Y(row_i);
    display_draw_string(8, ry + 8, label, C_DIM, C_BG, 1);
    uint16_t xs[4] = {102, 154, 206, 258};
    for (int i = 0; i < 4; i++)
    {
        bool a = (act == i);
        draw_btn(xs[i], SET_BTN_Y(row_i), 50, SET_BTN_H, o[i],
                 a ? C_SEL : C_BTN, a ? C_OK : C_DIM, a ? C_OK : C_BORDER);
    }
}

// ================================================================
// Barra de título e navegação
// ================================================================
static void draw_top_bar(const char *title, bool show_back, const char *right, uint16_t rc)
{
    display_draw_rect(0, 0, DISPLAY_WIDTH, UI_BAR_H, C_HEADER);
    display_draw_rect(0, UI_BAR_H - 1, DISPLAY_WIDTH, 1, C_BORDER);
    if (show_back)
    {
        display_draw_string(4, 8, "< VOLTAR", C_DIM, C_HEADER, 1);
        uint16_t tx = (DISPLAY_WIDTH - strlen(title) * 6) / 2;
        display_draw_string(tx, 8, title, C_TEXT, C_HEADER, 1);
    }
    else
    {
        display_draw_string(6, 8, title, C_TEXT, C_HEADER, 1);
    }
    if (right)
    {
        uint16_t rx = DISPLAY_WIDTH - strlen(right) * 6 - 4;
        display_draw_string(rx, 8, right, rc, C_HEADER, 1);
    }
}

static void draw_nav(ui_screen_t active)
{
    static const char *lbl[4] = {"STATUS", "MANUT.", "HIST.", "CONFIG."};
    static const ui_screen_t scr[4] = {SCREEN_STATUS, SCREEN_MAINTENANCE,
                                       SCREEN_HISTORY, SCREEN_SETTINGS};
    display_draw_rect(0, UI_NAV_Y, DISPLAY_WIDTH, UI_NAV_H, C_NAV_BG);
    display_draw_rect(0, UI_NAV_Y, DISPLAY_WIDTH, 1, C_BORDER);
    for (int i = 0; i < 4; i++)
    {
        uint16_t tx = i * UI_NAV_BTN_W;
        bool act = (active == scr[i]) || (i == 1 && active == SCREEN_REG_FORM) || (i == 3 && (active == SCREEN_SET_GENERAL || active == SCREEN_SET_VEHICLE || active == SCREEN_SET_ALERTS || active == SCREEN_SET_LEDS));
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

// ================================================================
// Gauge circular
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
    draw_top_bar("CONSOLE DE COMANDO - ASTRA", false, NULL, 0);
    struct
    {
        const char *lbl;
        uint16_t ic;
    } b[4] = {
        {"STATUS DO VEICULO", C_OK},
        {"MANUTENCAO", C_WARN},
        {"HIST. / ALERTAS", C_DIM},
        {"CONFIGURACOES", C_DIM},
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
            icon_car(ix, iy, b[i].ic);
            break;
        case 1:
            icon_wrench(ix, iy, b[i].ic);
            break;
        case 2:
            icon_clock(ix, iy, b[i].ic);
            break;
        case 3:
            icon_gear(ix, iy, b[i].ic);
            break;
        }
        uint16_t lw = strlen(b[i].lbl) * 6;
        display_draw_string(r->x + (r->w - lw) / 2, r->y + r->h - 16, b[i].lbl, b[i].ic, C_PANEL, 1);
    }
}

// ================================================================
// SCREEN_STATUS
// ================================================================
static void draw_status_frame(void)
{
    display_fill(C_BG);
    draw_top_bar("VEHICLE STATUS", false, NULL, 0);
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
static void draw_status_values(const ui_obd_t *o)
{
    char buf[28];
    snprintf(buf, sizeof(buf), "%ld KM", (long)o->total_km);
    display_draw_rect(6, UI_CONT_Y + 18, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 20, buf, C_TEXT, C_PANEL, 1);

    uint16_t tc = (o->temp_c >= 100) ? C_DUE : (o->temp_c >= 90) ? C_WARN
                                                                 : C_OK;
    snprintf(buf, sizeof(buf), "%d C  %s", o->temp_c, o->temp_c >= 100 ? "ALTO" : "NORMAL");
    display_draw_rect(6, UI_CONT_Y + 44, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 46, buf, tc, C_PANEL, 1);

    snprintf(buf, sizeof(buf), "%d HORAS", o->engine_hours);
    display_draw_rect(6, UI_CONT_Y + 70, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 72, buf, C_TEXT, C_PANEL, 1);

    snprintf(buf, sizeof(buf), "RPM:%d VEL:%dkm", o->rpm, o->speed_kmh);
    display_draw_rect(3, UI_CONT_Y + UI_CONT_H - 24, 150, 20, C_PANEL_D);
    display_draw_string(5, UI_CONT_Y + UI_CONT_H - 18, buf, C_DIM, C_PANEL_D, 1);

    display_draw_rect(165, UI_CONT_Y + 6, 144, UI_CONT_H - 28, C_PANEL_D);
    draw_gauge(237, UI_CONT_Y + 88, 52, 12, o->readiness_pct);
    snprintf(buf, sizeof(buf), "%d%%", o->readiness_pct);
    uint16_t tw = strlen(buf) * 12;
    display_draw_string(237 - tw / 2, UI_CONT_Y + 78, buf, C_TEXT, C_PANEL_D, 2);
}

// ================================================================
// SCREEN_MAINTENANCE
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
        uint16_t bg = act ? C_SEL : maint_bg_c(m->status);
        uint16_t bord = maint_fg(m->status);
        display_draw_rect(MAINT_ROW_X, ry, MAINT_ROW_W, MAINT_ROW_H, bg);
        display_draw_rect_border(MAINT_ROW_X, ry, MAINT_ROW_W, MAINT_ROW_H, bord, 1);
        display_draw_rect(MAINT_ROW_X, ry, 3, MAINT_ROW_H, bord);
        char name[12];
        strncpy(name, m->name, 11);
        name[11] = '\0';
        display_draw_string(MAINT_ROW_X + 6, ry + 4, name, C_TEXT, bg, 1);
        char sub[18];
        if (!m->valid)
            snprintf(sub, sizeof(sub), "sem registro");
        else if (m->km_remaining <= 0)
            snprintf(sub, sizeof(sub), "-%ld km", (long)(-m->km_remaining));
        else
            snprintf(sub, sizeof(sub), "+%ld km", (long)m->km_remaining);
        display_draw_string(MAINT_ROW_X + 6, ry + 14, sub, maint_fg(m->status), bg, 1);
        if (m->status != UI_MAINT_OK)
        {
            uint16_t dc = maint_fg(m->status);
            display_draw_rect(MAINT_ROW_W - 10, ry + 9, 6, 6, dc);
        }
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
    uint16_t ty = UI_CONT_Y + 4;
    display_draw_rect(dx, ty, dw, 14, C_SEL);
    uint16_t lx = dx + (dw - strlen(m->name) * 6) / 2;
    display_draw_string(lx, ty + 3, m->name, C_TEXT, C_SEL, 1);
    char buf[28];
    uint16_t vy = ty + 20;
#define DR(key, val, vc)                                         \
    {                                                            \
        display_draw_string(dx + 4, vy, key, C_DIM, C_PANEL, 1); \
        display_draw_string(dx + 92, vy, val, vc, C_PANEL, 1);   \
        vy += 13;                                                \
    }
    if (m->valid)
        snprintf(buf, sizeof(buf), "%.13s", m->last_date);
    else
        snprintf(buf, sizeof(buf), "---");
    DR("ULTIMA:", buf, C_TEXT);
    snprintf(buf, sizeof(buf), "%ld KM", (long)m->last_km);
    DR("KM TROCA:", buf, C_TEXT);
    snprintf(buf, sizeof(buf), "%ld KM", (long)m->interval_km);
    DR("INTERVALO:", buf, C_TEXT);
    snprintf(buf, sizeof(buf), "%ld KM", (long)m->next_km);
    DR("PROXIMA:", buf, fg);
    if (!m->valid)
        snprintf(buf, sizeof(buf), "SEM REGISTRO");
    else if (m->km_remaining <= 0)
        snprintf(buf, sizeof(buf), "VENC.%ldKM", (long)(-m->km_remaining));
    else
        snprintf(buf, sizeof(buf), "FALT.%ldKM", (long)m->km_remaining);
    DR("STATUS:", buf, fg);
#undef DR
    draw_progress(dx + 4, vy, dw - 8, 6, m->progress_pct, fg);
    draw_badge(dx + 4, vy + 10, m->status);
    draw_btn(MAINT_REG_BTN.x, MAINT_REG_BTN.y, MAINT_REG_BTN.w, MAINT_REG_BTN.h,
             "REALIZAR TROCA", C_PANEL_D, C_OK, C_OK);
}
static void draw_maintenance(const ui_dataset_t *d, uint8_t sel)
{
    display_fill(C_BG);
    uint8_t nb = 0;
    for (int i = 0; i < d->maint_n; i++)
        if (d->maint[i].status != UI_MAINT_OK)
            nb++;
    char rs[12] = "OK";
    uint16_t rc = C_OK;
    if (nb)
    {
        snprintf(rs, sizeof(rs), "%d ALRT.", nb);
        rc = C_WARN;
    }
    draw_top_bar("MANUTENCAO - REGISTRO", false, rs, rc);
    draw_maint_list(d, sel);
    draw_maint_detail(d, sel);
    draw_nav(SCREEN_MAINTENANCE);
}

// ================================================================
// SCREEN_HISTORY — com abas: HISTORICO | ALERTAS
// ================================================================
static void draw_hist_tabs(uint8_t tab)
{
    const char *labels[2] = {"HISTORICO", "ALERTAS"};
    for (int i = 0; i < 2; i++)
    {
        bool act = (tab == i);
        uint16_t bg = act ? C_NAV_ACT : C_NAV_BG;
        display_draw_rect(i * 160, UI_BAR_H, 160, HIST_TAB_H, bg);
        display_draw_rect_border(i * 160, UI_BAR_H, 160, HIST_TAB_H, C_BORDER, 1);
        if (act)
            display_draw_rect(i * 160, UI_BAR_H + HIST_TAB_H - 2, 160, 2, C_OK);
        uint16_t lx = i * 160 + (160 - strlen(labels[i]) * 6) / 2;
        display_draw_string(lx, UI_BAR_H + 6, labels[i], act ? C_TEXT : C_DIM, bg, 1);
    }
}

static void draw_historico_content(const ui_dataset_t *d)
{
    uint16_t hy = HIST_CONT_Y + 2;
    display_draw_rect(2, hy, DISPLAY_WIDTH - 4, 16, C_PANEL_D);
    display_draw_rect_border(2, hy, DISPLAY_WIDTH - 4, 16, C_BORDER, 1);
    display_draw_string(6, hy + 4, "DATA", C_DIM, C_PANEL_D, 1);
    display_draw_string(76, hy + 4, "ITEM", C_DIM, C_PANEL_D, 1);
    display_draw_string(196, hy + 4, "KM", C_DIM, C_PANEL_D, 1);
    display_draw_string(262, hy + 4, "STAT.", C_DIM, C_PANEL_D, 1);
    static const uint16_t cx[3] = {72, 192, 258};
    for (int i = 0; i < 3; i++)
        display_draw_rect(cx[i], hy, 1, 16, C_BORDER);
    hy += 18;
    if (d->hist_n == 0)
    {
        display_draw_string(10, hy + 20, "Nenhum registro salvo.", C_DIM, C_BG, 1);
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
        for (int c = 0; c < 3; c++)
            display_draw_rect(cx[c], hy, 1, 18, C_BORDER);
        hy += 20;
    }
}

static void draw_alertas_content(const ui_dataset_t *d)
{
    uint16_t y = HIST_CONT_Y + 4;
    if (d->alert_n == 0)
    {
        display_draw_string(10, y + 30, "Nenhum alerta ativo.", C_DIM, C_BG, 1);
        return;
    }
    // Dois passes: crítico primeiro, depois warning
    for (int pass = 0; pass < 2; pass++)
    {
        ui_alert_level_t tgt = (pass == 0) ? UI_ALERT_CRITICAL : UI_ALERT_WARNING;
        for (int i = 0; i < d->alert_n; i++)
        {
            if (d->alerts[i].level != tgt)
                continue;
            if (y + 26 > UI_NAV_Y)
                break;
            const ui_alert_row_t *a = &d->alerts[i];
            uint16_t bg = (tgt == UI_ALERT_CRITICAL) ? C_CRIT_BG : C_WARN_BG;
            uint16_t bord = (tgt == UI_ALERT_CRITICAL) ? C_DUE : C_WARN;
            display_draw_rect(4, y, DISPLAY_WIDTH - 8, 24, bg);
            display_draw_rect_border(4, y, DISPLAY_WIDTH - 8, 24, bord, 1);
            display_draw_rect(4, y, 5, 24, bord);
            icon_tri(12, y + 3, bord);
            display_draw_string(36, y + 8, a->text, bord, bg, 1);
            y += 28;
        }
    }
}

static void draw_history(const ui_dataset_t *d, uint8_t tab)
{
    display_fill(C_BG);
    draw_top_bar("HISTORICO DE MANUTENCAO", false, NULL, 0);
    draw_hist_tabs(tab);
    if (tab == 0)
        draw_historico_content(d);
    else
        draw_alertas_content(d);
    draw_nav(SCREEN_HISTORY);
}

// ================================================================
// SCREEN_SETTINGS — lista de categorias
// ================================================================
static void draw_settings_home(void)
{
    display_fill(C_BG);
    draw_top_bar("CONFIGURACOES", false, NULL, 0);

    struct
    {
        const char *title, *sub;
        uint16_t ic;
    } cats[4] = {
        {"GERAL", "Idioma, brilho, tela", C_DIM},
        {"VEICULO / MANUT", "Intervalos, calibrar", C_WARN},
        {"ALERTAS", "Som, tipo de notif.", C_DUE},
        {"LEDs", "Modo, cor, efeitos", C_OK},
    };
    for (int i = 0; i < 4; i++)
    {
        const ui_region_t *r = &SET_CAT[i];
        display_draw_rect(r->x, r->y, r->w, r->h, C_PANEL);
        display_draw_rect_border(r->x, r->y, r->w, r->h, C_BORDER, 1);
        display_draw_rect(r->x, r->y, 4, r->h, cats[i].ic); // borda esquerda colorida
        uint16_t ix = r->x + 10, iy = r->y + (r->h - 20) / 2;
        switch (i)
        {
        case 0:
            icon_gear(ix, iy, cats[i].ic);
            break;
        case 1:
            icon_wrench(ix, iy, cats[i].ic);
            break;
        case 2:
            icon_tri(ix, iy + 1, cats[i].ic);
            break;
        case 3:
            icon_led(ix, iy, cats[i].ic);
            break;
        }
        display_draw_string(r->x + 36, r->y + 8, cats[i].title, cats[i].ic, C_PANEL, 1);
        display_draw_string(r->x + 36, r->y + 22, cats[i].sub, C_DIM, C_PANEL, 1);
        display_draw_string(DISPLAY_WIDTH - 14, r->y + (r->h - 7) / 2, ">", C_DIM, C_PANEL, 1);
    }
    draw_nav(SCREEN_SETTINGS);
}

// ================================================================
// SCREEN_SET_GENERAL
// ================================================================
static void draw_set_general(const app_settings_t *s)
{
    display_fill(C_BG);
    draw_top_bar("GERAL", true, NULL, 0);

    draw_set_tog2(0, "Unidade:", "KM", "MILHAS", s->units == 0);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", s->brightness);
    draw_set_inc(1, "Brilho:", buf, C_OK);

    static const char *off_labels[] = {"NUNCA", "30s", "60s", "2min", "5min"};
    static const uint16_t off_vals[] = {0, 30, 60, 120, 300};
    uint8_t off_idx = 0;
    for (int i = 0; i < 5; i++)
        if (s->screen_off_s == off_vals[i])
            off_idx = i;
    draw_set_inc(2, "Tela off:", off_labels[off_idx], C_DIM);

    // Botão calibrar touch
    display_draw_string(8, SET_ROW_Y(3) + 8, "Calibrar touch:", C_DIM, C_BG, 1);
    draw_btn(102, SET_BTN_Y(3), DISPLAY_WIDTH - 106, SET_BTN_H,
             "INICIAR CALIBRACAO", C_PANEL_D, C_WARN, C_WARN);

    draw_nav(SCREEN_SETTINGS);
}

// ================================================================
// SCREEN_SET_VEHICLE
// ================================================================
static void draw_set_vehicle(const ui_dataset_t *d, uint8_t sel)
{
    display_fill(C_BG);
    draw_top_bar("VEICULO / MANUTENCAO", true, NULL, 0);

    for (int i = 0; i < d->maint_n && i < UI_MAX_MAINT; i++)
    {
        const ui_maint_row_t *m = &d->maint[i];
        uint16_t ry = VEH_ROW_Y(i);
        bool act = (i == sel);
        uint16_t bg = act ? C_SEL : C_PANEL_D;
        display_draw_rect(2, ry, DISPLAY_WIDTH - 4, VEH_ROW_H, bg);
        display_draw_rect_border(2, ry, DISPLAY_WIDTH - 4, VEH_ROW_H, act ? C_OK : C_BORDER, 1);
        char name[14];
        strncpy(name, m->name, 13);
        name[13] = '\0';
        display_draw_string(6, ry + 6, name, C_TEXT, bg, 1);
        // Intervalo atual alinhado à direita
        char iv[10];
        snprintf(iv, sizeof(iv), "%ldkm", (long)m->interval_km);
        uint16_t iw = strlen(iv) * 6;
        display_draw_string(DISPLAY_WIDTH - iw - 6, ry + 6, iv,
                            act ? C_OK : C_DIM, bg, 1);
    }

    // Linha de edição (abaixo da lista)
    if (sel < d->maint_n)
    {
        const ui_maint_row_t *m = &d->maint[sel];
        char val[12];
        snprintf(val, sizeof(val), "%ld KM", (long)m->interval_km);
        uint16_t ey = VEH_EDIT_Y;
        display_draw_rect(0, ey, DISPLAY_WIDTH, UI_NAV_Y - ey, C_HEADER);
        display_draw_rect(0, ey, DISPLAY_WIDTH, 1, C_BORDER);
        draw_btn(2, ey + 2, 44, 24, "-10k", C_BTN, C_DIM, C_BORDER);
        draw_btn(48, ey + 2, 36, 24, "-1k", C_BTN, C_DIM, C_BORDER);
        display_draw_rect(86, ey + 2, 150, 24, C_PANEL_D);
        display_draw_rect_border(86, ey + 2, 150, 24, C_OK, 1);
        uint16_t vx = 86 + (150 - strlen(val) * 6) / 2;
        display_draw_string(vx, ey + 10, val, C_OK, C_PANEL_D, 1);
        draw_btn(238, ey + 2, 36, 24, "+1k", C_BTN, C_DIM, C_BORDER);
        draw_btn(276, ey + 2, 42, 24, "+10k", C_BTN, C_DIM, C_BORDER);
    }

    draw_nav(SCREEN_SETTINGS);
}

// ================================================================
// SCREEN_SET_ALERTS
// ================================================================
static void draw_set_alerts(const app_settings_t *s)
{
    display_fill(C_BG);
    draw_top_bar("ALERTAS", true, NULL, 0);
    draw_set_tog2(0, "Som:", "SIM", "NAO", s->alert_sound);
    static const char *tipos[3] = {"VISUAL", "SONORO", "AMBOS"};
    draw_set_tog3(1, "Tipo:", tipos, s->alert_type);
    // Placeholder futuro
    display_draw_string(8, SET_ROW_Y(2) + 8, "(volume - futuro)", C_DIM, C_BG, 1);
    draw_nav(SCREEN_SETTINGS);
}

// ================================================================
// SCREEN_SET_LEDS
// ================================================================
static void draw_set_leds(const app_settings_t *s)
{
    display_fill(C_BG);
    draw_top_bar("LEDs", true, NULL, 0);

    draw_set_tog2(0, "LEDs:", "LIGAR", "DESLIGAR", s->led_enabled);

    static const char *modos[4] = {"FIXO", "RESP", "REAT", "CARRO"};
    draw_set_tog4(1, "Modo:", modos, s->led_mode);

    char buf[8];
    snprintf(buf, sizeof(buf), "%d%%", s->led_brightness);
    draw_set_inc(2, "Brilho:", buf, C_OK);

    snprintf(buf, sizeof(buf), "%d%%", s->led_speed);
    draw_set_inc(3, "Velocid.:", buf, C_DIM);

    // Linha R G B
    uint16_t rgb_y = SET_ROW_Y(4);
    display_draw_string(8, rgb_y + 8, "Cor RGB:", C_DIM, C_BG, 1);

    // R
    display_draw_string(102, rgb_y + 8, "R", RGB(200, 50, 50), C_BG, 1);
    draw_btn(112, SET_BTN_Y(4), 22, SET_BTN_H, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(136, SET_BTN_Y(4), 26, SET_BTN_H, C_PANEL_D);
    display_draw_rect_border(136, SET_BTN_Y(4), 26, SET_BTN_H, C_BORDER, 1);
    char rv[4];
    snprintf(rv, sizeof(rv), "%d", s->led_r);
    display_draw_string(138, SET_BTN_Y(4) + 8, rv, C_TEXT, C_PANEL_D, 1);
    draw_btn(164, SET_BTN_Y(4), 22, SET_BTN_H, "+", C_BTN, C_DIM, C_BORDER);

    // G
    display_draw_string(192, rgb_y + 8, "G", RGB(50, 200, 50), C_BG, 1);
    draw_btn(202, SET_BTN_Y(4), 22, SET_BTN_H, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(226, SET_BTN_Y(4), 26, SET_BTN_H, C_PANEL_D);
    display_draw_rect_border(226, SET_BTN_Y(4), 26, SET_BTN_H, C_BORDER, 1);
    char gv[4];
    snprintf(gv, sizeof(gv), "%d", s->led_g);
    display_draw_string(228, SET_BTN_Y(4) + 8, gv, C_TEXT, C_PANEL_D, 1);
    draw_btn(254, SET_BTN_Y(4), 22, SET_BTN_H, "+", C_BTN, C_DIM, C_BORDER);

    // B
    display_draw_string(282, rgb_y + 8, "B", RGB(50, 100, 220), C_BG, 1);
    // (fora do espaço — cabe apenas em row 5 extra)
    // Put B on next implicit row since width runs out
    uint16_t b_y = SET_ROW_Y(4) + 30;
    draw_btn(102, b_y, 22, 22, "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(126, b_y, 26, 22, C_PANEL_D);
    display_draw_rect_border(126, b_y, 26, 22, C_BORDER, 1);
    char bv[4];
    snprintf(bv, sizeof(bv), "%d", s->led_b);
    display_draw_string(128, b_y + 7, bv, C_TEXT, C_PANEL_D, 1);
    draw_btn(154, b_y, 22, 22, "+", C_BTN, C_DIM, C_BORDER);
    display_draw_string(182, b_y + 7, "(B)", RGB(50, 100, 220), C_BG, 1);

    draw_nav(SCREEN_SETTINGS);
}

// ================================================================
// SCREEN_REG_FORM
// ================================================================
static void draw_form_values(const ui_ctx_t *ctx)
{
    char buf[12];
    snprintf(buf, sizeof(buf), "%ld KM", (long)ctx->form_km);
    const ui_region_t *kv = &FORM_KM_BTN[2];
    display_draw_rect(kv->x + 1, kv->y + 1, kv->w - 2, kv->h - 2, C_PANEL_D);
    uint16_t lx = kv->x + (kv->w - strlen(buf) * 6) / 2;
    display_draw_string(lx, kv->y + 10, buf, C_OK, C_PANEL_D, 1);

    snprintf(buf, sizeof(buf), "%02d", ctx->form_day);
    display_draw_rect(FORM_DAY_BTN[1].x + 1, FORM_DAY_BTN[1].y + 1,
                      FORM_DAY_BTN[1].w - 2, FORM_DAY_BTN[1].h - 2, C_PANEL_D);
    display_draw_string(FORM_DAY_BTN[1].x + 9, FORM_DAY_BTN[1].y + 10, buf, C_TEXT, C_PANEL_D, 1);

    snprintf(buf, sizeof(buf), "%02d", ctx->form_month);
    display_draw_rect(FORM_MON_BTN[1].x + 1, FORM_MON_BTN[1].y + 1,
                      FORM_MON_BTN[1].w - 2, FORM_MON_BTN[1].h - 2, C_PANEL_D);
    display_draw_string(FORM_MON_BTN[1].x + 9, FORM_MON_BTN[1].y + 10, buf, C_TEXT, C_PANEL_D, 1);

    snprintf(buf, sizeof(buf), "%04d", (int)ctx->form_year);
    display_draw_rect(FORM_YR_BTN[1].x + 1, FORM_YR_BTN[1].y + 1,
                      FORM_YR_BTN[1].w - 2, FORM_YR_BTN[1].h - 2, C_PANEL_D);
    display_draw_string(FORM_YR_BTN[1].x + 6, FORM_YR_BTN[1].y + 10, buf, C_TEXT, C_PANEL_D, 1);
}

static void draw_reg_form(const ui_ctx_t *ctx)
{
    display_fill(C_BG);
    const ui_maint_row_t *m = &ctx->data->maint[ctx->maint_sel];
    char title[40];
    snprintf(title, sizeof(title), "REGISTRAR: %.18s", m->name);
    draw_top_bar(title, false, NULL, 0);

    display_draw_string(4, UI_CONT_Y + 8, "QUILOMETRAGEM:", C_DIM, C_BG, 1);
    draw_btn(FORM_KM_BTN[0].x, FORM_KM_BTN[0].y, FORM_KM_BTN[0].w, FORM_KM_BTN[0].h,
             "-10k", C_BTN, C_DIM, C_BORDER);
    draw_btn(FORM_KM_BTN[1].x, FORM_KM_BTN[1].y, FORM_KM_BTN[1].w, FORM_KM_BTN[1].h,
             "-1k", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_KM_BTN[2].x, FORM_KM_BTN[2].y,
                      FORM_KM_BTN[2].w, FORM_KM_BTN[2].h, C_PANEL_D);
    display_draw_rect_border(FORM_KM_BTN[2].x, FORM_KM_BTN[2].y,
                             FORM_KM_BTN[2].w, FORM_KM_BTN[2].h, C_OK, 1);
    draw_btn(FORM_KM_BTN[3].x, FORM_KM_BTN[3].y, FORM_KM_BTN[3].w, FORM_KM_BTN[3].h,
             "+1k", C_BTN, C_DIM, C_BORDER);
    draw_btn(FORM_KM_BTN[4].x, FORM_KM_BTN[4].y, FORM_KM_BTN[4].w, FORM_KM_BTN[4].h,
             "+10k", C_BTN, C_DIM, C_BORDER);

    display_draw_string(4, FORM_DATE_Y - 16, "DATA (DD / MM / AAAA):", C_DIM, C_BG, 1);
    display_draw_rect(96, FORM_DATE_Y - 2, 1, FORM_DATE_H + 4, C_BORDER);
    display_draw_rect(198, FORM_DATE_Y - 2, 1, FORM_DATE_H + 4, C_BORDER);

    draw_btn(FORM_DAY_BTN[0].x, FORM_DAY_BTN[0].y, FORM_DAY_BTN[0].w, FORM_DAY_BTN[0].h,
             "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_DAY_BTN[1].x, FORM_DAY_BTN[1].y,
                      FORM_DAY_BTN[1].w, FORM_DAY_BTN[1].h, C_PANEL_D);
    display_draw_rect_border(FORM_DAY_BTN[1].x, FORM_DAY_BTN[1].y,
                             FORM_DAY_BTN[1].w, FORM_DAY_BTN[1].h, C_BORDER, 1);
    draw_btn(FORM_DAY_BTN[2].x, FORM_DAY_BTN[2].y, FORM_DAY_BTN[2].w, FORM_DAY_BTN[2].h,
             "+", C_BTN, C_DIM, C_BORDER);
    draw_btn(FORM_MON_BTN[0].x, FORM_MON_BTN[0].y, FORM_MON_BTN[0].w, FORM_MON_BTN[0].h,
             "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_MON_BTN[1].x, FORM_MON_BTN[1].y,
                      FORM_MON_BTN[1].w, FORM_MON_BTN[1].h, C_PANEL_D);
    display_draw_rect_border(FORM_MON_BTN[1].x, FORM_MON_BTN[1].y,
                             FORM_MON_BTN[1].w, FORM_MON_BTN[1].h, C_BORDER, 1);
    draw_btn(FORM_MON_BTN[2].x, FORM_MON_BTN[2].y, FORM_MON_BTN[2].w, FORM_MON_BTN[2].h,
             "+", C_BTN, C_DIM, C_BORDER);
    draw_btn(FORM_YR_BTN[0].x, FORM_YR_BTN[0].y, FORM_YR_BTN[0].w, FORM_YR_BTN[0].h,
             "-", C_BTN, C_DIM, C_BORDER);
    display_draw_rect(FORM_YR_BTN[1].x, FORM_YR_BTN[1].y,
                      FORM_YR_BTN[1].w, FORM_YR_BTN[1].h, C_PANEL_D);
    display_draw_rect_border(FORM_YR_BTN[1].x, FORM_YR_BTN[1].y,
                             FORM_YR_BTN[1].w, FORM_YR_BTN[1].h, C_BORDER, 1);
    draw_btn(FORM_YR_BTN[2].x, FORM_YR_BTN[2].y, FORM_YR_BTN[2].w, FORM_YR_BTN[2].h,
             "+", C_BTN, C_DIM, C_BORDER);

    display_draw_rect(2, FORM_DATE_Y + FORM_DATE_H + 8, DISPLAY_WIDTH - 4, 1, C_BORDER);
    draw_btn(FORM_CONFIRM.x, FORM_CONFIRM.y, FORM_CONFIRM.w, FORM_CONFIRM.h,
             "CONFIRMAR", C_PANEL_D, C_OK, C_OK);
    draw_btn(FORM_CANCEL.x, FORM_CANCEL.y, FORM_CANCEL.w, FORM_CANCEL.h,
             "CANCELAR", C_PANEL_D, C_DUE, C_DUE);

    draw_form_values(ctx);
}

// ================================================================
// API pública
// ================================================================
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

void ui_init(ui_ctx_t *ctx, ui_dataset_t *data,
             void (*on_register)(uint8_t, int32_t, uint8_t, uint8_t, uint16_t, void *),
             void (*on_settings)(const app_settings_t *, void *),
             void (*on_interval)(uint8_t, int32_t, void *),
             void (*on_calibrate)(void *),
             void *userdata)
{
    ctx->screen = SCREEN_HOME;
    ctx->maint_sel = 0;
    ctx->hist_tab = 0;
    ctx->set_veh_sel = 0;
    ctx->data = data;
    ctx->redraw = UI_REDRAW_FULL;
    ctx->last_rpm = 0xFFFF;
    ctx->last_spd = 0xFFFF;
    ctx->last_tmp = 0xFF;
    ctx->last_fuel = 0xFF;
    ctx->settings = data->settings;
    ctx->form_km = data->obd.total_km;
    ctx->form_day = 1;
    ctx->form_month = 1;
    ctx->form_year = 2025;
    ctx->on_register = on_register;
    ctx->on_settings = on_settings;
    ctx->on_interval = on_interval;
    ctx->on_calibrate = on_calibrate;
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

bool ui_handle_touch(ui_ctx_t *ctx, int16_t px, int16_t py)
{
    bool changed = false;

    // Nav bar (todos exceto HOME e REG_FORM)
    if (ctx->screen != SCREEN_HOME && ctx->screen != SCREEN_REG_FORM)
    {
        static const ui_screen_t nav_dest[4] = {
            SCREEN_STATUS, SCREEN_MAINTENANCE, SCREEN_HISTORY, SCREEN_SETTINGS};
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
                    ui_screen_t dest[4] = {SCREEN_STATUS, SCREEN_MAINTENANCE,
                                           SCREEN_HISTORY, SCREEN_SETTINGS};
                    ctx->screen = dest[i];
                    changed = true;
                    break;
                }
            }
            break;

        case SCREEN_MAINTENANCE:
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
            if (!changed && ui_hit(&MAINT_REG_BTN, px, py))
            {
                ctx->form_km = ctx->data->obd.total_km;
                ctx->screen = SCREEN_REG_FORM;
                changed = true;
            }
            break;

        case SCREEN_HISTORY:
            for (int i = 0; i < 2; i++)
            {
                if (ui_hit(&HIST_TAB[i], px, py) && ctx->hist_tab != (uint8_t)i)
                {
                    ctx->hist_tab = i;
                    changed = true;
                    break;
                }
            }
            break;

        case SCREEN_SETTINGS:
            for (int i = 0; i < 4; i++)
            {
                if (ui_hit(&SET_CAT[i], px, py))
                {
                    ui_screen_t dest[4] = {SCREEN_SET_GENERAL, SCREEN_SET_VEHICLE,
                                           SCREEN_SET_ALERTS, SCREEN_SET_LEDS};
                    ctx->screen = dest[i];
                    changed = true;
                    break;
                }
            }
            break;

        // ----------------------------------------------------------------
        case SCREEN_SET_GENERAL:
        {
            bool sc = false;
            if (ui_hit(&BACK_REGION, px, py))
            {
                ctx->screen = SCREEN_SETTINGS;
                changed = true;
                break;
            }
            // Unidade
            if (TAPPED(102, SET_BTN_Y(0), 102, SET_BTN_H))
            {
                ctx->settings.units = 0;
                sc = true;
            }
            else if (TAPPED(206, SET_BTN_Y(0), 110, SET_BTN_H))
            {
                ctx->settings.units = 1;
                sc = true;
            }
            // Brilho
            else if (TAPPED(SET_MINUS_X, SET_BTN_Y(1), SET_MINUS_W, SET_BTN_H))
            {
                ctx->settings.brightness = clamp8(ctx->settings.brightness - 10, 20, 100);
                sc = true;
            }
            else if (TAPPED(SET_PLUS_X, SET_BTN_Y(1), SET_PLUS_W, SET_BTN_H))
            {
                ctx->settings.brightness = clamp8(ctx->settings.brightness + 10, 20, 100);
                sc = true;
            }
            // Tela off — cicla entre valores
            else if (TAPPED(SET_MINUS_X, SET_BTN_Y(2), SET_MINUS_W, SET_BTN_H) ||
                     TAPPED(SET_PLUS_X, SET_BTN_Y(2), SET_PLUS_W, SET_BTN_H))
            {
                static const uint16_t cyc[] = {0, 30, 60, 120, 300};
                uint8_t idx = 0;
                for (int i = 0; i < 5; i++)
                    if (ctx->settings.screen_off_s == cyc[i])
                    {
                        idx = i;
                        break;
                    }
                if (TAPPED(SET_MINUS_X, SET_BTN_Y(2), SET_MINUS_W, SET_BTN_H))
                    idx = (idx == 0) ? 4 : idx - 1;
                else
                    idx = (idx == 4) ? 0 : idx + 1;
                ctx->settings.screen_off_s = cyc[idx];
                sc = true;
            }
            // Calibrar
            else if (TAPPED(102, SET_BTN_Y(3), DISPLAY_WIDTH - 106, SET_BTN_H))
            {
                if (ctx->on_calibrate)
                    ctx->on_calibrate(ctx->userdata);
                // calibrate é bloqueante no main — não muda tela
            }
            if (sc)
            {
                if (ctx->on_settings)
                    ctx->on_settings(&ctx->settings, ctx->userdata);
                changed = true;
            }
            break;
        }

        // ----------------------------------------------------------------
        case SCREEN_SET_VEHICLE:
        {
            if (ui_hit(&BACK_REGION, px, py))
            {
                ctx->screen = SCREEN_SETTINGS;
                changed = true;
                break;
            }
            // Seleção de item
            for (int i = 0; i < ctx->data->maint_n; i++)
            {
                if (TAPPED(2, VEH_ROW_Y(i), DISPLAY_WIDTH - 4, VEH_ROW_H))
                {
                    if (ctx->set_veh_sel != (uint8_t)i)
                    {
                        ctx->set_veh_sel = i;
                        changed = true;
                    }
                    break;
                }
            }
            // Edição de intervalo do item selecionado
            uint8_t si = ctx->set_veh_sel;
            if (si < ctx->data->maint_n)
            {
                int32_t cur = ctx->data->maint[si].interval_km;
                int32_t delta = 0;
                if (TAPPED(2, VEH_EDIT_Y, 44, 24))
                    delta = -10000;
                else if (TAPPED(48, VEH_EDIT_Y, 36, 24))
                    delta = -1000;
                else if (TAPPED(238, VEH_EDIT_Y, 36, 24))
                    delta = +1000;
                else if (TAPPED(276, VEH_EDIT_Y, 42, 24))
                    delta = +10000;
                if (delta)
                {
                    int32_t nv = clamp32(cur + delta, 1000, 200000);
                    if (ctx->on_interval)
                        ctx->on_interval(si, nv, ctx->userdata);
                    // A atualização real do dataset ocorre em update_dataset() no main
                    changed = true;
                }
            }
            break;
        }

        // ----------------------------------------------------------------
        case SCREEN_SET_ALERTS:
        {
            if (ui_hit(&BACK_REGION, px, py))
            {
                ctx->screen = SCREEN_SETTINGS;
                changed = true;
                break;
            }
            bool sc = false;
            if (TAPPED(102, SET_BTN_Y(0), 102, SET_BTN_H))
            {
                ctx->settings.alert_sound = true;
                sc = true;
            }
            else if (TAPPED(206, SET_BTN_Y(0), 110, SET_BTN_H))
            {
                ctx->settings.alert_sound = false;
                sc = true;
            }
            else if (TAPPED(102, SET_BTN_Y(1), 68, SET_BTN_H))
            {
                ctx->settings.alert_type = 0;
                sc = true;
            }
            else if (TAPPED(172, SET_BTN_Y(1), 68, SET_BTN_H))
            {
                ctx->settings.alert_type = 1;
                sc = true;
            }
            else if (TAPPED(242, SET_BTN_Y(1), 68, SET_BTN_H))
            {
                ctx->settings.alert_type = 2;
                sc = true;
            }
            if (sc)
            {
                if (ctx->on_settings)
                    ctx->on_settings(&ctx->settings, ctx->userdata);
                changed = true;
            }
            break;
        }

        // ----------------------------------------------------------------
        case SCREEN_SET_LEDS:
        {
            if (ui_hit(&BACK_REGION, px, py))
            {
                ctx->screen = SCREEN_SETTINGS;
                changed = true;
                break;
            }
            bool sc = false;
            // ON/OFF
            if (TAPPED(102, SET_BTN_Y(0), 102, SET_BTN_H))
            {
                ctx->settings.led_enabled = true;
                sc = true;
            }
            else if (TAPPED(206, SET_BTN_Y(0), 110, SET_BTN_H))
            {
                ctx->settings.led_enabled = false;
                sc = true;
            }
            // Modo (4 opções)
            else if (TAPPED(102, SET_BTN_Y(1), 50, SET_BTN_H))
            {
                ctx->settings.led_mode = 0;
                sc = true;
            }
            else if (TAPPED(154, SET_BTN_Y(1), 50, SET_BTN_H))
            {
                ctx->settings.led_mode = 1;
                sc = true;
            }
            else if (TAPPED(206, SET_BTN_Y(1), 50, SET_BTN_H))
            {
                ctx->settings.led_mode = 2;
                sc = true;
            }
            else if (TAPPED(258, SET_BTN_Y(1), 50, SET_BTN_H))
            {
                ctx->settings.led_mode = 3;
                sc = true;
            }
            // Brilho
            else if (TAPPED(SET_MINUS_X, SET_BTN_Y(2), SET_MINUS_W, SET_BTN_H))
            {
                ctx->settings.led_brightness = clamp8(ctx->settings.led_brightness - 10, 0, 100);
                sc = true;
            }
            else if (TAPPED(SET_PLUS_X, SET_BTN_Y(2), SET_PLUS_W, SET_BTN_H))
            {
                ctx->settings.led_brightness = clamp8(ctx->settings.led_brightness + 10, 0, 100);
                sc = true;
            }
            // Velocidade
            else if (TAPPED(SET_MINUS_X, SET_BTN_Y(3), SET_MINUS_W, SET_BTN_H))
            {
                ctx->settings.led_speed = clamp8(ctx->settings.led_speed - 10, 10, 100);
                sc = true;
            }
            else if (TAPPED(SET_PLUS_X, SET_BTN_Y(3), SET_PLUS_W, SET_BTN_H))
            {
                ctx->settings.led_speed = clamp8(ctx->settings.led_speed + 10, 10, 100);
                sc = true;
            }
            // R [-][+]
            else if (TAPPED(112, SET_BTN_Y(4), 22, SET_BTN_H))
            {
                ctx->settings.led_r = clamp8(ctx->settings.led_r - 16, 0, 255);
                sc = true;
            }
            else if (TAPPED(164, SET_BTN_Y(4), 22, SET_BTN_H))
            {
                ctx->settings.led_r = clamp8(ctx->settings.led_r + 16, 0, 255);
                sc = true;
            }
            // G [-][+]
            else if (TAPPED(202, SET_BTN_Y(4), 22, SET_BTN_H))
            {
                ctx->settings.led_g = clamp8(ctx->settings.led_g - 16, 0, 255);
                sc = true;
            }
            else if (TAPPED(254, SET_BTN_Y(4), 22, SET_BTN_H))
            {
                ctx->settings.led_g = clamp8(ctx->settings.led_g + 16, 0, 255);
                sc = true;
            }
            // B [-][+] (linha extra)
            else if (TAPPED(102, SET_ROW_Y(4) + 30, 22, 22))
            {
                ctx->settings.led_b = clamp8(ctx->settings.led_b - 16, 0, 255);
                sc = true;
            }
            else if (TAPPED(154, SET_ROW_Y(4) + 30, 22, 22))
            {
                ctx->settings.led_b = clamp8(ctx->settings.led_b + 16, 0, 255);
                sc = true;
            }
            if (sc)
            {
                if (ctx->on_settings)
                    ctx->on_settings(&ctx->settings, ctx->userdata);
                changed = true;
            }
            break;
        }

        // ----------------------------------------------------------------
        case SCREEN_REG_FORM:
        {
            bool fc = false;
            if (ui_hit(&FORM_KM_BTN[0], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km - 10000, 0, 999999);
                fc = true;
            }
            else if (ui_hit(&FORM_KM_BTN[1], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km - 1000, 0, 999999);
                fc = true;
            }
            else if (ui_hit(&FORM_KM_BTN[3], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km + 1000, 0, 999999);
                fc = true;
            }
            else if (ui_hit(&FORM_KM_BTN[4], px, py))
            {
                ctx->form_km = clamp32(ctx->form_km + 10000, 0, 999999);
                fc = true;
            }
            else if (ui_hit(&FORM_DAY_BTN[0], px, py))
            {
                ctx->form_day = clamp8(ctx->form_day - 1, 1, 31);
                fc = true;
            }
            else if (ui_hit(&FORM_DAY_BTN[2], px, py))
            {
                ctx->form_day = clamp8(ctx->form_day + 1, 1, 31);
                fc = true;
            }
            else if (ui_hit(&FORM_MON_BTN[0], px, py))
            {
                ctx->form_month = clamp8(ctx->form_month - 1, 1, 12);
                fc = true;
            }
            else if (ui_hit(&FORM_MON_BTN[2], px, py))
            {
                ctx->form_month = clamp8(ctx->form_month + 1, 1, 12);
                fc = true;
            }
            else if (ui_hit(&FORM_YR_BTN[0], px, py))
            {
                ctx->form_year = clamp16(ctx->form_year - 1, 2000, 2099);
                fc = true;
            }
            else if (ui_hit(&FORM_YR_BTN[2], px, py))
            {
                ctx->form_year = clamp16(ctx->form_year + 1, 2000, 2099);
                fc = true;
            }
            else if (ui_hit(&FORM_CONFIRM, px, py))
            {
                if (ctx->on_register)
                    ctx->on_register(ctx->maint_sel, ctx->form_km,
                                     ctx->form_day, ctx->form_month, ctx->form_year,
                                     ctx->userdata);
                ctx->screen = SCREEN_MAINTENANCE;
                changed = true;
            }
            else if (ui_hit(&FORM_CANCEL, px, py))
            {
                ctx->screen = SCREEN_MAINTENANCE;
                changed = true;
            }
            if (fc)
            {
                if (!(ctx->redraw & UI_REDRAW_FULL))
                    ctx->redraw |= UI_REDRAW_FORM;
                return true;
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
        case SCREEN_HISTORY:
            draw_history(ctx->data, ctx->hist_tab);
            break;
        case SCREEN_SETTINGS:
            draw_settings_home();
            break;
        case SCREEN_REG_FORM:
            draw_reg_form(ctx);
            break;
        case SCREEN_SET_GENERAL:
            draw_set_general(&ctx->settings);
            break;
        case SCREEN_SET_VEHICLE:
            draw_set_vehicle(ctx->data, ctx->set_veh_sel);
            break;
        case SCREEN_SET_ALERTS:
            draw_set_alerts(&ctx->settings);
            break;
        case SCREEN_SET_LEDS:
            draw_set_leds(&ctx->settings);
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
