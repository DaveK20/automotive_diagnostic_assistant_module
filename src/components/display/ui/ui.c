#include "components/display/ui/ui.h"
#include "components/display/display.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ================================================================
// Paleta militar (olive/dark green)
// ================================================================
#define C_BG RGB(42, 46, 22)      // Fundo geral
#define C_PANEL RGB(62, 68, 32)   // Painel médio
#define C_PANEL_D RGB(32, 35, 16) // Painel escuro
#define C_BORDER RGB(88, 96, 42)  // Borda padrão
#define C_HEADER RGB(28, 31, 14)  // Fundo de título
#define C_SEL RGB(80, 88, 40)     // Item selecionado
#define C_TEXT COLOR_WHITE
#define C_DIM RGB(168, 178, 96)     // Texto secundário
#define C_OK RGB(45, 185, 45)       // Verde
#define C_WARN RGB(220, 140, 0)     // Laranja
#define C_DUE RGB(210, 30, 30)      // Vermelho
#define C_CRIT_BG RGB(48, 8, 8)     // Fundo crítico
#define C_WARN_BG RGB(48, 30, 5)    // Fundo alerta
#define C_INFO_BG RGB(38, 42, 20)   // Fundo info
#define C_NAV_BG RGB(22, 25, 11)    // Fundo nav bar
#define C_NAV_ACT RGB(72, 80, 36)   // Aba ativa
#define C_GAUGE_DIM RGB(28, 32, 14) // Arco apagado gauge

// ================================================================
// Helpers geométricos
// ================================================================

// Triângulo com display_draw_line (para ícone de alerta)
static void draw_triangle(int16_t x0, int16_t y0,
                          int16_t x1, int16_t y1,
                          int16_t x2, int16_t y2, uint16_t c)
{
    display_draw_line(x0, y0, x1, y1, c);
    display_draw_line(x1, y1, x2, y2, c);
    display_draw_line(x2, y2, x0, y0, c);
}

// Ícone de carro (~22×14 px), canto sup-esq (ix, iy)
static void draw_icon_car(uint16_t ix, uint16_t iy, uint16_t c)
{
    display_draw_rect(ix + 4, iy + 1, 14, 5, c);  // teto
    display_draw_rect(ix + 1, iy + 5, 20, 7, c);  // corpo
    display_draw_rect(ix + 2, iy + 11, 6, 3, c);  // roda esq
    display_draw_rect(ix + 14, iy + 11, 6, 3, c); // roda dir
    // janelas
    display_draw_rect(ix + 5, iy + 2, 4, 3, C_PANEL_D);
    display_draw_rect(ix + 13, iy + 2, 4, 3, C_PANEL_D);
}

// Ícone de chave (~20×20 px)
static void draw_icon_wrench(uint16_t ix, uint16_t iy, uint16_t c)
{
    // cabo diagonal
    display_draw_line(ix + 4, iy + 16, ix + 16, iy + 4, c);
    display_draw_line(ix + 5, iy + 16, ix + 17, iy + 4, c);
    display_draw_line(ix + 4, iy + 15, ix + 16, iy + 3, c);
    // cabeça
    display_draw_rect(ix + 13, iy + 1, 6, 6, c);
    display_draw_rect(ix + 14, iy + 2, 4, 4, C_PANEL_D);
    // ponta
    display_draw_rect(ix + 1, iy + 14, 5, 5, c);
    display_draw_rect(ix + 2, iy + 15, 3, 3, C_PANEL_D);
}

// Ícone de alerta (~20×18 px)
static void draw_icon_alert(uint16_t ix, uint16_t iy, uint16_t c)
{
    draw_triangle(ix + 10, iy, ix + 20, iy + 17, ix, iy + 17, c);
    display_draw_rect(ix + 9, iy + 6, 3, 6, C_PANEL_D); // !
    display_draw_rect(ix + 9, iy + 14, 3, 2, C_PANEL_D);
}

// Ícone de relógio (~20×20 px)
static void draw_icon_clock(uint16_t ix, uint16_t iy, uint16_t c)
{
    // círculo (approximado com rects)
    display_draw_rect(ix + 4, iy + 1, 12, 2, c);
    display_draw_rect(ix + 2, iy + 3, 16, 2, c);
    display_draw_rect(ix + 1, iy + 5, 18, 10, c);
    display_draw_rect(ix + 2, iy + 15, 16, 2, c);
    display_draw_rect(ix + 4, iy + 17, 12, 2, c);
    // interior escuro
    display_draw_rect(ix + 3, iy + 3, 14, 13, C_PANEL_D);
    // ponteiros
    display_draw_line(ix + 10, iy + 5, ix + 10, iy + 9, c); // hora
    display_draw_line(ix + 10, iy + 9, ix + 14, iy + 9, c); // min
}

// ================================================================
// Barra de título
// ================================================================
static void draw_top_bar(const char *title, const char *right, uint16_t r_color)
{
    display_draw_rect(0, 0, DISPLAY_WIDTH, UI_BAR_H, C_HEADER);
    display_draw_rect(0, UI_BAR_H - 1, DISPLAY_WIDTH, 1, C_BORDER);
    display_draw_string(6, 8, title, C_TEXT, C_HEADER, 1);
    if (right)
    {
        uint16_t rx = DISPLAY_WIDTH - strlen(right) * 6 - 4;
        display_draw_string(rx, 8, right, r_color, C_HEADER, 1);
    }
}

// ================================================================
// Barra de navegação inferior (4 abas)
// Nav mapeamento: 0=STATUS 1=MANUTENCAO 2=ALERTAS 3=HISTORICO
// SCREEN_HOME não tem nav bar
// ================================================================
static void draw_nav(ui_screen_t active)
{
    static const char *labels[4] = {"STATUS", "MANUT.", "ALERT.", "HIST."};
    static const ui_screen_t screens[4] = {
        SCREEN_STATUS, SCREEN_MAINTENANCE, SCREEN_ALERTS, SCREEN_HISTORY};

    display_draw_rect(0, UI_NAV_Y, DISPLAY_WIDTH, UI_NAV_H, C_NAV_BG);
    display_draw_rect(0, UI_NAV_Y, DISPLAY_WIDTH, 1, C_BORDER);

    for (int i = 0; i < 4; i++)
    {
        uint16_t tx = i * UI_NAV_BTN_W;
        bool act = (active == screens[i]) ||
                   (i == 1 && active == SCREEN_CONFIRM_REG);
        uint16_t bg = act ? C_NAV_ACT : C_NAV_BG;

        display_draw_rect(tx, UI_NAV_Y, UI_NAV_BTN_W, UI_NAV_H, bg);
        if (act)
            display_draw_rect(tx, UI_NAV_Y, UI_NAV_BTN_W, 2, C_OK);

        uint16_t lx = tx + (UI_NAV_BTN_W - strlen(labels[i]) * 6) / 2;
        display_draw_string(lx, UI_NAV_Y + 10, labels[i],
                            act ? C_TEXT : C_DIM, bg, 1);
    }
    // Divisores verticais
    for (int i = 1; i < 4; i++)
        display_draw_rect(i * UI_NAV_BTN_W, UI_NAV_Y + 2, 1, UI_NAV_H - 4, C_BORDER);
}

// ================================================================
// Badge de status (OK / ATENC / VENC)
// ================================================================
static void draw_badge(uint16_t x, uint16_t y, ui_maint_status_t s)
{
    uint16_t fg = (s == UI_MAINT_DUE) ? C_DUE : (s == UI_MAINT_WARN) ? C_WARN
                                                                     : C_OK;
    uint16_t bg = (s == UI_MAINT_DUE) ? C_CRIT_BG : (s == UI_MAINT_WARN) ? C_WARN_BG
                                                                         : C_PANEL_D;
    const char *t = (s == UI_MAINT_DUE) ? "VENC" : (s == UI_MAINT_WARN) ? "ATEN"
                                                                        : "OK";
    uint16_t w = strlen(t) * 6 + 6;
    display_draw_rect(x, y, w, 12, bg);
    display_draw_rect_border(x, y, w, 12, fg, 1);
    display_draw_string(x + 3, y + 2, t, fg, bg, 1);
}

// ================================================================
// Barra de progresso
// ================================================================
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

// ================================================================
// GAUGE circular (tela STATUS)
// Centro (cx, cy), raio externo r, espessura t, pct 0–100
// ================================================================
static void draw_gauge(int16_t cx, int16_t cy, uint8_t r, uint8_t t, uint8_t pct)
{
    // Arco de 210° a 450° (CCW via topo), 240° de amplitude
    float filled = 210.0f + pct * 2.4f;
    uint8_t ri = r - t;

    for (int deg = 210; deg <= 450; deg += 3)
    {
        float a = deg * 0.01745329f;
        float ca = cosf(a), sa = sinf(a);

        int pos = deg - 210;
        bool active = (deg <= (int)filled);
        uint16_t c;
        if (!active)
            c = C_GAUGE_DIM;
        else if (pos < 80)
            c = C_OK;
        else if (pos < 160)
            c = C_WARN;
        else
            c = C_DUE;

        int16_t x0 = cx + (int16_t)(ri * ca);
        int16_t y0 = cy - (int16_t)(ri * sa);
        int16_t x1 = cx + (int16_t)(r * ca);
        int16_t y1 = cy - (int16_t)(r * sa);
        display_draw_line(x0, y0, x1, y1, c);
        display_draw_line(x0 + 1, y0, x1 + 1, y1, c); // 2px espessura
    }

    // Agulha
    float na = filled * 0.01745329f;
    int16_t nx = cx + (int16_t)((ri - 2) * cosf(na));
    int16_t ny = cy - (int16_t)((ri - 2) * sinf(na));
    display_draw_line(cx, cy, nx, ny, C_TEXT);
    display_draw_line(cx + 1, cy, nx + 1, ny, C_TEXT);
    // Ponto central
    display_draw_rect(cx - 3, cy - 3, 6, 6, C_BORDER);
    display_draw_rect(cx - 2, cy - 2, 4, 4, C_DIM);
}

// ================================================================
// SCREEN_HOME — 4 botões grandes em 2×2
// ================================================================
static void draw_home(void)
{
    display_fill(C_BG);
    draw_top_bar("CONSOLE DE COMANDO - ASTRA", NULL, 0);

    struct
    {
        const char *label;
        uint16_t icon_color;
    } btns[4] = {
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
        display_draw_rect(r->x + 1, r->y + 1, r->w - 2, 3, btns[i].icon_color); // barra topo colorida

        uint16_t ic_x = r->x + (r->w - 22) / 2;
        uint16_t ic_y = r->y + 18;

        switch (i)
        {
        case 0:
            draw_icon_car(ic_x, ic_y, btns[i].icon_color);
            break;
        case 1:
            draw_icon_wrench(ic_x, ic_y, btns[i].icon_color);
            break;
        case 2:
            draw_icon_alert(ic_x, ic_y + 2, btns[i].icon_color);
            break;
        case 3:
            draw_icon_clock(ic_x, ic_y, btns[i].icon_color);
            break;
        }

        // Label centrado no fundo do botão
        uint16_t lw = strlen(btns[i].label) * 6;
        uint16_t lx = r->x + (r->w - lw) / 2;
        display_draw_string(lx, r->y + r->h - 16, btns[i].label,
                            btns[i].icon_color, C_PANEL, 1);
    }
}

// ================================================================
// SCREEN_STATUS — dados OBD (frame estático)
// ================================================================
static void draw_status_frame(const ui_dataset_t *d)
{
    display_fill(C_BG);
    draw_top_bar("VEHICLE STATUS", NULL, 0);

    // Painel esquerdo (x=2..155)
    display_draw_rect(2, UI_CONT_Y + 2, 152, UI_CONT_H - 4, C_PANEL);
    display_draw_rect_border(2, UI_CONT_Y + 2, 152, UI_CONT_H - 4, C_BORDER, 1);

    display_draw_string(8, UI_CONT_Y + 8, "QUILOMETRAGEM", C_DIM, C_PANEL, 1);
    display_draw_string(8, UI_CONT_Y + 34, "TEMP. MOTOR", C_DIM, C_PANEL, 1);
    display_draw_string(8, UI_CONT_Y + 60, "TEMPO DE USO", C_DIM, C_PANEL, 1);

    // Linha separadora bottom-strip RPM|VEL
    display_draw_rect(2, UI_CONT_Y + UI_CONT_H - 26, 152, 1, C_BORDER);

    // Painel direito (x=158..318) — fundo do gauge
    display_draw_rect(158, UI_CONT_Y + 2, 158, UI_CONT_H - 4, C_PANEL_D);
    display_draw_rect_border(158, UI_CONT_Y + 2, 158, UI_CONT_H - 4, C_BORDER, 1);
    display_draw_string(170, UI_CONT_Y + 148, "PRONTO PARA", C_DIM, C_PANEL_D, 1);
    display_draw_string(174, UI_CONT_Y + 160, "SERVICO", C_DIM, C_PANEL_D, 1);

    draw_nav(SCREEN_STATUS);
}

// Só os valores dinâmicos — sem display_fill
static void draw_status_values(const ui_obd_t *o)
{
    char buf[24];

    // --- KM ---
    snprintf(buf, sizeof(buf), "%ld KM", (long)o->total_km);
    display_draw_rect(6, UI_CONT_Y + 18, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 20, buf, C_TEXT, C_PANEL, 1);

    // --- Temperatura ---
    uint16_t tc = (o->temp_c >= 100) ? C_DUE : (o->temp_c >= 90) ? C_WARN
                                                                 : C_OK;
    snprintf(buf, sizeof(buf), "%d C  %s",
             o->temp_c, o->temp_c >= 100 ? "ALTO" : "NORMAL");
    display_draw_rect(6, UI_CONT_Y + 44, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 46, buf, tc, C_PANEL, 1);

    // --- Horas ---
    snprintf(buf, sizeof(buf), "%d HORAS", o->engine_hours);
    display_draw_rect(6, UI_CONT_Y + 70, 142, 14, C_PANEL);
    display_draw_string(8, UI_CONT_Y + 72, buf, C_TEXT, C_PANEL, 1);

    // --- RPM | VEL strip ---
    char strip[32];
    snprintf(strip, sizeof(strip), "RPM:%d  VEL:%dkm/h", o->rpm, o->speed_kmh);
    display_draw_rect(3, UI_CONT_Y + UI_CONT_H - 24, 150, 20, C_PANEL_D);
    display_draw_string(5, UI_CONT_Y + UI_CONT_H - 18, strip, C_DIM, C_PANEL_D, 1);

    // --- Gauge (readiness) ---
    // Fundo interior do gauge
    display_draw_rect(165, UI_CONT_Y + 6, 144, UI_CONT_H - 28, C_PANEL_D);
    draw_gauge(237, UI_CONT_Y + 88, 52, 12, o->readiness_pct);

    // Percentual no centro
    snprintf(buf, sizeof(buf), "%d%%", o->readiness_pct);
    uint16_t tw = strlen(buf) * 12;
    display_draw_string(237 - tw / 2, UI_CONT_Y + 78, buf, C_TEXT, C_PANEL_D, 2);
}

// ================================================================
// SCREEN_MAINTENANCE — lista esquerda (134px) + detalhe direita
// ================================================================
static uint16_t status_color_ui(ui_maint_status_t s); // forward decl

static void draw_maint_list(const ui_dataset_t *d, uint8_t sel)
{
    // Painel esquerdo
    display_draw_rect(0, UI_CONT_Y, MAINT_LIST_W + 2, UI_CONT_H, C_PANEL_D);
    display_draw_rect(MAINT_LIST_W + 1, UI_CONT_Y, 1, UI_CONT_H, C_BORDER);

    for (int i = 0; i < d->maint_n && i < UI_MAX_MAINT; i++)
    {
        const ui_maint_row_t *m = &d->maint[i];
        uint16_t ry = MAINT_ROW_Y(i);
        if (ry + MAINT_ROW_H > UI_NAV_Y)
            break;

        bool active = (i == sel);
        uint16_t bg = active ? C_SEL : (m->status == UI_MAINT_DUE) ? C_CRIT_BG
                                   : (m->status == UI_MAINT_WARN)  ? C_WARN_BG
                                                                   : C_PANEL;
        uint16_t bord = (m->status == UI_MAINT_DUE) ? C_DUE : (m->status == UI_MAINT_WARN) ? C_WARN
                                                                                           : C_BORDER;

        display_draw_rect(MAINT_ROW_X, ry, MAINT_ROW_W, MAINT_ROW_H, bg);
        display_draw_rect_border(MAINT_ROW_X, ry, MAINT_ROW_W, MAINT_ROW_H, bord, 1);
        if (active)
            display_draw_rect(MAINT_ROW_X, ry, 3, MAINT_ROW_H, C_OK);

        // Nome (truncado para caber)
        char name[14];
        strncpy(name, m->name, 13);
        name[13] = '\0';
        display_draw_string(MAINT_ROW_X + 6, ry + 4, name,
                            m->status == UI_MAINT_DUE ? C_DUE : C_TEXT, bg, 1);

        // Badge compacto
        uint16_t bx = MAINT_ROW_W - 25;
        draw_badge(bx, ry + 6, m->status);
    }
}

static void draw_maint_detail(const ui_dataset_t *d, uint8_t sel)
{
    if (sel >= d->maint_n)
        return;
    const ui_maint_row_t *m = &d->maint[sel];

    uint16_t dx = MAINT_LIST_W + 4;
    uint16_t dw = DISPLAY_WIDTH - dx - 2;

    // Fundo do painel direito
    display_draw_rect(dx, UI_CONT_Y, dw, UI_CONT_H, C_PANEL);
    display_draw_rect_border(dx, UI_CONT_Y, dw, UI_CONT_H, C_BORDER, 1);

    // Título do item
    uint16_t ty = UI_CONT_Y + 4;
    display_draw_rect(dx, ty, dw, 14, C_SEL);
    uint16_t lx = dx + (dw - strlen(m->name) * 6) / 2;
    display_draw_string(lx, ty + 3, m->name, C_TEXT, C_SEL, 1);

    // Dados
    char buf[28];
    uint16_t vy = ty + 20;
    uint16_t vl = dx + 4;  // label x
    uint16_t vv = dx + 96; // value x
    uint16_t sc = status_color_ui(m->status);
#define ROW(label, value, vc)                                  \
    {                                                          \
        display_draw_string(vl, vy, label, C_DIM, C_PANEL, 1); \
        display_draw_string(vv, vy, value, vc, C_PANEL, 1);    \
        vy += 13;                                              \
    }

    if (m->valid)
        snprintf(buf, sizeof(buf), "%s", m->last_date);
    else
        snprintf(buf, sizeof(buf), "-");
    ROW("ULTIMA:", buf, C_TEXT);

    snprintf(buf, sizeof(buf), "%ld KM", (long)m->last_km);
    ROW("KM TROCA:", buf, C_TEXT);

    snprintf(buf, sizeof(buf), "%ld KM", (long)m->interval_km);
    ROW("INTERVALO:", buf, C_TEXT);

    snprintf(buf, sizeof(buf), "%ld KM", (long)m->next_km);
    ROW("PROXIMA:", buf, sc);

    if (m->km_remaining <= 0)
        snprintf(buf, sizeof(buf), "VENC. %ld KM", (long)(-m->km_remaining));
    else
        snprintf(buf, sizeof(buf), "FALTAM %ld KM", (long)m->km_remaining);
    ROW("STATUS:", buf, sc);
#undef ROW

    // Barra de progresso
    display_draw_rect(vl, vy, dw - 8, 6, C_PANEL_D);
    draw_progress(vl, vy, dw - 8, 6, m->progress_pct, sc);
    vy += 10;

    // Botão REALIZAR TROCA
    uint16_t btn_y = UI_NAV_Y - 28;
    display_draw_rect(dx + 1, btn_y, dw - 2, 24, C_PANEL_D);
    display_draw_rect_border(dx + 1, btn_y, dw - 2, 24, C_OK, 1);
    uint16_t btx = dx + (dw - 14 * 6) / 2;
    display_draw_string(btx, btn_y + 8, "REALIZAR TROCA", C_OK, C_PANEL_D, 1);
}

static uint16_t status_color_ui(ui_maint_status_t s)
{
    switch (s)
    {
    case UI_MAINT_OK:
        return C_OK;
    case UI_MAINT_WARN:
        return C_WARN;
    case UI_MAINT_DUE:
        return C_DUE;
    }
    return C_DIM;
}

static void draw_maintenance(const ui_dataset_t *d, uint8_t sel)
{
    display_fill(C_BG);

    uint32_t alerts = 0;
    for (int i = 0; i < d->maint_n; i++)
        if (d->maint[i].status != UI_MAINT_OK)
            alerts++;
    char hdr[32];
    snprintf(hdr, sizeof(hdr), "MANUTENCAO%s",
             alerts ? " (!) " : "");
    draw_top_bar("MANUTENCAO - REGISTRO DE SERVICOS",
                 alerts ? "ALERTA" : "OK",
                 alerts ? C_DUE : C_OK);

    draw_maint_list(d, sel);
    draw_maint_detail(d, sel);
    draw_nav(SCREEN_MAINTENANCE);
}

// ================================================================
// SCREEN_ALERTS — alertas empilhados
// ================================================================
static void draw_alerts(const ui_dataset_t *d)
{
    display_fill(C_BG);
    draw_top_bar("ALERTAS E LEMBRETES", NULL, 0);

    uint16_t y = UI_CONT_Y + 4;

    if (d->alert_n == 0)
    {
        display_draw_string(10, y + 40, "Nenhum alerta ativo.", C_DIM, C_BG, 1);
        draw_nav(SCREEN_ALERTS);
        return;
    }

    for (int i = 0; i < d->alert_n && i < UI_MAX_ALERTS; i++)
    {
        const ui_alert_row_t *a = &d->alerts[i];
        if (y + 28 > UI_NAV_Y)
            break;

        uint16_t bg = (a->level == UI_ALERT_CRITICAL) ? C_CRIT_BG : (a->level == UI_ALERT_WARNING) ? C_WARN_BG
                                                                                                   : C_INFO_BG;
        uint16_t bord = (a->level == UI_ALERT_CRITICAL) ? C_DUE : (a->level == UI_ALERT_WARNING) ? C_WARN
                                                                                                 : C_BORDER;
        uint16_t fc = (a->level == UI_ALERT_CRITICAL) ? C_DUE : (a->level == UI_ALERT_WARNING) ? C_WARN
                                                                                               : C_DIM;

        display_draw_rect(4, y, DISPLAY_WIDTH - 8, 26, bg);
        display_draw_rect_border(4, y, DISPLAY_WIDTH - 8, 26, bord, 1);
        // Borda esquerda grossa
        display_draw_rect(4, y, 4, 26, bord);

        // Ícone
        if (a->level == UI_ALERT_CRITICAL || a->level == UI_ALERT_WARNING)
            draw_icon_alert(10, y + 4, fc);
        else
            draw_icon_clock(10, y + 4, fc);

        display_draw_string(36, y + 8, a->text, fc, bg, 1);
        y += 30;
    }

    draw_nav(SCREEN_ALERTS);
}

// ================================================================
// SCREEN_HISTORY — tabela
// ================================================================
static void draw_history(const ui_dataset_t *d)
{
    display_fill(C_BG);
    draw_top_bar("HISTORICO DE MANUTENCAO", NULL, 0);

    // Cabeçalho da tabela
    uint16_t hy = UI_CONT_Y + 2;
    display_draw_rect(2, hy, DISPLAY_WIDTH - 4, 16, C_PANEL_D);
    display_draw_rect_border(2, hy, DISPLAY_WIDTH - 4, 16, C_BORDER, 1);
    display_draw_string(6, hy + 4, "DATA", C_DIM, C_PANEL_D, 1);
    display_draw_string(76, hy + 4, "ITEM", C_DIM, C_PANEL_D, 1);
    display_draw_string(196, hy + 4, "KM", C_DIM, C_PANEL_D, 1);
    display_draw_string(260, hy + 4, "STATUS", C_DIM, C_PANEL_D, 1);

    // Divisores coluna
    display_draw_rect(72, hy, 1, 16, C_BORDER);
    display_draw_rect(192, hy, 1, 16, C_BORDER);
    display_draw_rect(256, hy, 1, 16, C_BORDER);

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

        // Barra colorida esquerda
        display_draw_rect(2, hy, 3, 18, C_OK);

        display_draw_string(6, hy + 5, h->date, C_DIM, bg, 1);
        display_draw_string(76, hy + 5, h->item, C_TEXT, bg, 1);

        char km_fmt[16];
        snprintf(km_fmt, sizeof(km_fmt), "%s KM", h->km);
        display_draw_string(196, hy + 5, km_fmt, C_DIM, bg, 1);

        // Checkmark OK
        display_draw_string(268, hy + 5, "OK", C_OK, bg, 1);

        // Divisores coluna
        display_draw_rect(72, hy, 1, 18, C_BORDER);
        display_draw_rect(192, hy, 1, 18, C_BORDER);
        display_draw_rect(256, hy, 1, 18, C_BORDER);

        hy += 20;
    }

    draw_nav(SCREEN_HISTORY);
}

// ================================================================
// Modal de confirmação
// ================================================================
static void draw_confirm_modal(const ui_dataset_t *d, uint8_t sel)
{
    if (sel >= d->maint_n)
        return;
    const ui_maint_row_t *m = &d->maint[sel];

    // Overlay
    display_draw_rect(14, 70, DISPLAY_WIDTH - 28, 110, C_PANEL_D);
    display_draw_rect_border(14, 70, DISPLAY_WIDTH - 28, 110, C_BORDER, 2);

    display_draw_string(20, 78, "CONFIRMAR REGISTRO?", C_TEXT, C_PANEL_D, 1);
    display_draw_string(20, 92, m->name, C_OK, C_PANEL_D, 1);
    display_draw_string(20, 108, "Registrar no hodometro", C_DIM, C_PANEL_D, 1);
    display_draw_string(20, 120, "atual e salvar?", C_DIM, C_PANEL_D, 1);

    // SIM
    display_draw_rect(CONFIRM_YES.x, CONFIRM_YES.y, CONFIRM_YES.w, CONFIRM_YES.h, C_PANEL_D);
    display_draw_rect_border(CONFIRM_YES.x, CONFIRM_YES.y, CONFIRM_YES.w, CONFIRM_YES.h, C_OK, 1);
    display_draw_string(CONFIRM_YES.x + 36, CONFIRM_YES.y + 8, "SIM", C_OK, C_PANEL_D, 1);

    // NAO
    display_draw_rect(CONFIRM_NO.x, CONFIRM_NO.y, CONFIRM_NO.w, CONFIRM_NO.h, C_PANEL_D);
    display_draw_rect_border(CONFIRM_NO.x, CONFIRM_NO.y, CONFIRM_NO.w, CONFIRM_NO.h, C_DUE, 1);
    display_draw_string(CONFIRM_NO.x + 36, CONFIRM_NO.y + 8, "NAO", C_DUE, C_PANEL_D, 1);
}

// ================================================================
// API pública
// ================================================================

void ui_init(ui_ctx_t *ctx, ui_dataset_t *data,
             void (*on_register)(uint8_t, void *), void *userdata)
{
    ctx->screen = SCREEN_HOME;
    ctx->maint_sel = 0;
    ctx->data = data;
    ctx->redraw = UI_REDRAW_FULL;
    ctx->last_rpm = 0xFFFF;
    ctx->last_spd = 0xFFFF;
    ctx->last_tmp = 0xFF;
    ctx->last_fuel = 0xFF;
    ctx->on_register = on_register;
    ctx->userdata = userdata;
}

void ui_update_obd(ui_ctx_t *ctx)
{
    if (ctx->screen != SCREEN_STATUS)
        return;

    const ui_obd_t *o = &ctx->data->obd;
    if (o->rpm == ctx->last_rpm &&
        o->speed_kmh == ctx->last_spd &&
        o->temp_c == ctx->last_tmp &&
        o->fuel_pct == ctx->last_fuel)
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

    // Navegação inferior (todas as telas exceto HOME e modal)
    if (ctx->screen != SCREEN_HOME && ctx->screen != SCREEN_CONFIRM_REG)
    {
        for (int i = 0; i < 4; i++)
        {
            if (ui_hit(&UI_NAV[i], px, py))
            {
                ui_screen_t dest = (ui_screen_t)(SCREEN_STATUS + i);
                if (ctx->screen != dest)
                {
                    ctx->screen = dest;
                    changed = true;
                }
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
            for (int i = 0; i < ctx->data->maint_n && i < UI_MAX_MAINT; i++)
            {
                ui_region_t r = {MAINT_ROW_X, MAINT_ROW_Y(i), MAINT_ROW_W, MAINT_ROW_H};
                if (ui_hit(&r, px, py))
                {
                    if (ctx->maint_sel != i)
                    {
                        ctx->maint_sel = i;
                        changed = true;
                    }
                    break;
                }
            }
            // Botão REALIZAR TROCA (na área direita, linha inferior)
            {
                uint16_t btn_y = UI_NAV_Y - 28;
                ui_region_t rbtn = {MAINT_LIST_W + 4, btn_y, DISPLAY_WIDTH - MAINT_LIST_W - 6, 24};
                if (ui_hit(&rbtn, px, py))
                {
                    ctx->screen = SCREEN_CONFIRM_REG;
                    changed = true;
                }
            }
            break;

        case SCREEN_CONFIRM_REG:
            if (ui_hit(&CONFIRM_YES, px, py))
            {
                if (ctx->on_register)
                    ctx->on_register(ctx->maint_sel, ctx->userdata);
                ctx->screen = SCREEN_MAINTENANCE;
                changed = true;
            }
            else if (ui_hit(&CONFIRM_NO, px, py))
            {
                ctx->screen = SCREEN_MAINTENANCE;
                changed = true;
            }
            break;

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
            draw_status_frame(ctx->data);
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
        case SCREEN_CONFIRM_REG:
            draw_maintenance(ctx->data, ctx->maint_sel);
            draw_confirm_modal(ctx->data, ctx->maint_sel);
            break;
        }
    }
    else if (ctx->redraw & UI_REDRAW_DATA)
    {
        // Só atualiza valores OBD sem redesenhar o frame
        draw_status_values(&ctx->data->obd);
    }

    ctx->redraw = UI_REDRAW_NONE;
}