#include "calibration.h"
#include "display.h"
#include "nvs_flash.h"
#include "nvs.h"
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include <stdio.h>
#include <string.h>
#include <math.h>

// ---------------------------------------------------------------
// NVS — namespace e chave
// ---------------------------------------------------------------
#define CAL_NVS_NAMESPACE "display"
#define CAL_NVS_KEY "touch_cal"

// ---------------------------------------------------------------
// Posições dos 3 alvos de calibração na tela (pixels)
// Formam um triângulo que cobre bem a área útil:
//   P0 — superior esquerdo
//   P1 — superior direito
//   P2 — inferior centro
// ---------------------------------------------------------------
#define N_CAL_POINTS 3

static const uint16_t CAL_SCREEN_X[N_CAL_POINTS] = {30, 290, 160};
static const uint16_t CAL_SCREEN_Y[N_CAL_POINTS] = {25, 25, 215};

// ---------------------------------------------------------------
// Quantas amostras bruta coleta por ponto
// ---------------------------------------------------------------
#define CAL_SAMPLES 40

// ---------------------------------------------------------------
// Helpers visuais
// ---------------------------------------------------------------

// Desenha um alvo (+) com círculo central
static void draw_target(uint16_t x, uint16_t y, uint16_t color)
{
    // Linhas do crosshair
    display_draw_line(x - 18, y, x + 18, y, color);
    display_draw_line(x, y - 18, x, y + 18, color);
    // Quadrado central 5×5
    display_draw_rect(x - 3, y - 3, 7, 7, color);
}

// Apaga o alvo (desenha por cima com preto)
static void erase_target(uint16_t x, uint16_t y)
{
    display_draw_line(x - 18, y, x + 18, y, COLOR_BLACK);
    display_draw_line(x, y - 18, x, y + 18, COLOR_BLACK);
    display_draw_rect(x - 3, y - 3, 7, 7, COLOR_BLACK);
}

// Tela inicial de calibração
static void draw_cal_screen(void)
{
    display_fill(COLOR_BLACK);
    display_draw_rect(0, 0, DISPLAY_WIDTH, 28, COLOR_NAVY);
    display_draw_string(8, 8, "CALIBRACAO DO TOUCH", COLOR_WHITE, COLOR_NAVY, 2);
    display_draw_string(10, 38, "Pressione cada alvo (+)", COLOR_YELLOW, COLOR_BLACK, 1);
    display_draw_string(10, 52, "com a caneta ate confirmar", COLOR_YELLOW, COLOR_BLACK, 1);
}

// Mensagem de status na parte inferior
static void status_msg(const char *msg, uint16_t color)
{
    display_draw_rect(0, DISPLAY_HEIGHT - 22, DISPLAY_WIDTH, 22, COLOR_BLACK);
    display_draw_string(10, DISPLAY_HEIGHT - 18, msg, color, COLOR_BLACK, 1);
}

// ---------------------------------------------------------------
// Coleta amostras brutas em um ponto
// ---------------------------------------------------------------
static bool collect_raw_point(uint16_t screen_x, uint16_t screen_y,
                              uint8_t point_num,
                              uint16_t *rx_out, uint16_t *ry_out)
{
    char buf[48];
    snprintf(buf, sizeof(buf), "Ponto %d/3 — segure o alvo", point_num);
    status_msg(buf, COLOR_CYAN);

    // Aguarda pressionar
    while (!touch_is_pressed())
        vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelay(pdMS_TO_TICKS(80)); // debounce inicial

    // Coleta amostras e acumula
    uint32_t rx_sum = 0, ry_sum = 0;
    uint16_t count = 0;

    while (count < CAL_SAMPLES)
    {
        uint16_t rx, ry;
        if (touch_get_raw(&rx, &ry))
        {
            rx_sum += rx;
            ry_sum += ry;
            count++;

            // Barra de progresso visual
            uint16_t bar_w = (uint16_t)((uint32_t)count * 200 / CAL_SAMPLES);
            display_draw_rect(60, DISPLAY_HEIGHT - 10, bar_w, 6, COLOR_GREEN);
        }
        vTaskDelay(pdMS_TO_TICKS(15));
    }

    // Aguarda soltar
    status_msg("Solte...", COLOR_GRAY);
    while (touch_is_pressed())
        vTaskDelay(pdMS_TO_TICKS(10));
    vTaskDelay(pdMS_TO_TICKS(300)); // debounce pós-toque

    if (count == 0)
        return false;

    *rx_out = (uint16_t)(rx_sum / count);
    *ry_out = (uint16_t)(ry_sum / count);

    printf("[CAL] Ponto %d: tela=(%d,%d)  raw=(%d,%d)  amostras=%d\n",
           point_num, screen_x, screen_y, *rx_out, *ry_out, count);

    return true;
}

// ---------------------------------------------------------------
// Resolve o sistema afim com 3 pontos via Regra de Cramer
//
// Para um eixo de saída S = [s0, s1, s2] e entradas raw:
//   M = | rx0 ry0 1 |
//       | rx1 ry1 1 |
//       | rx2 ry2 1 |
//
//   M * [A, B, C]^T = S
//   → A, B, C = coef. por Cramer
// ---------------------------------------------------------------
static bool solve_affine(
    float rx0, float ry0,
    float rx1, float ry1,
    float rx2, float ry2,
    float sx0, float sx1, float sx2,
    float sy0, float sy1, float sy2,
    float *ax, float *bx, float *cx,
    float *ay, float *by, float *cy)
{
    // Determinante da matriz M
    float det = rx0 * (ry1 - ry2) - ry0 * (rx1 - rx2) + (rx1 * ry2 - rx2 * ry1);

    if (fabsf(det) < 1.0f)
    {
        printf("[CAL] ERRO: pontos colineares (det=%.2f) — refaça a calibração\n", det);
        return false;
    }

    // --- Coeficientes para X ---
    *ax = (sx0 * (ry1 - ry2) - ry0 * (sx1 - sx2) + (sx1 * ry2 - sx2 * ry1)) / det;
    *bx = (rx0 * (sx1 - sx2) - sx0 * (rx1 - rx2) + (rx1 * sx2 - rx2 * sx1)) / det;
    *cx = (rx0 * (ry1 * sx2 - sx1 * ry2) - ry0 * (rx1 * sx2 - sx1 * rx2) + sx0 * (rx1 * ry2 - ry1 * rx2)) / det;

    // --- Coeficientes para Y ---
    *ay = (sy0 * (ry1 - ry2) - ry0 * (sy1 - sy2) + (sy1 * ry2 - sy2 * ry1)) / det;
    *by = (rx0 * (sy1 - sy2) - sy0 * (rx1 - rx2) + (rx1 * sy2 - rx2 * sy1)) / det;
    *cy = (rx0 * (ry1 * sy2 - sy1 * ry2) - ry0 * (rx1 * sy2 - sy1 * rx2) + sy0 * (rx1 * ry2 - ry1 * rx2)) / det;

    printf("[CAL] Coef X: ax=%.4f bx=%.4f cx=%.2f\n", *ax, *bx, *cx);
    printf("[CAL] Coef Y: ay=%.4f by=%.4f cy=%.2f\n", *ay, *by, *cy);

    return true;
}

// ---------------------------------------------------------------
// API PÚBLICA
// ---------------------------------------------------------------

esp_err_t calibration_run(touch_cal_t *cal)
{
    uint16_t rx[N_CAL_POINTS], ry[N_CAL_POINTS];

    draw_cal_screen();
    vTaskDelay(pdMS_TO_TICKS(500));

    // Coleta os 3 pontos
    for (int i = 0; i < N_CAL_POINTS; i++)
    {
        draw_target(CAL_SCREEN_X[i], CAL_SCREEN_Y[i], COLOR_RED);

        bool ok = collect_raw_point(CAL_SCREEN_X[i], CAL_SCREEN_Y[i],
                                    i + 1, &rx[i], &ry[i]);
        if (!ok)
        {
            status_msg("Falha na leitura. Reinicie.", COLOR_RED);
            cal->valid = false;
            return ESP_FAIL;
        }

        // Confirma visualmente o ponto coletado
        erase_target(CAL_SCREEN_X[i], CAL_SCREEN_Y[i]);
        draw_target(CAL_SCREEN_X[i], CAL_SCREEN_Y[i], COLOR_GREEN);
        vTaskDelay(pdMS_TO_TICKS(400));
    }

    // Calcula coeficientes
    bool ok = solve_affine(
        rx[0], ry[0], rx[1], ry[1], rx[2], ry[2],
        CAL_SCREEN_X[0], CAL_SCREEN_X[1], CAL_SCREEN_X[2],
        CAL_SCREEN_Y[0], CAL_SCREEN_Y[1], CAL_SCREEN_Y[2],
        &cal->ax, &cal->bx, &cal->cx,
        &cal->ay, &cal->by, &cal->cy);

    if (!ok)
    {
        display_fill(COLOR_BLACK);
        display_draw_string(20, 100, "ERRO: pontos invalidos!", COLOR_RED, COLOR_BLACK, 2);
        display_draw_string(20, 125, "Reinicie e tente novamente", COLOR_GRAY, COLOR_BLACK, 1);
        cal->valid = false;
        return ESP_FAIL;
    }

    cal->valid = true;

    // Salva no NVS
    esp_err_t err = calibration_save(cal);

    // Tela de sucesso
    display_fill(COLOR_BLACK);
    display_draw_rect(0, 0, DISPLAY_WIDTH, 28, COLOR_DARK_GREEN);
    display_draw_string(50, 8, "Calibracao OK!", COLOR_WHITE, COLOR_DARK_GREEN, 2);
    display_draw_string(20, 50, "Tela calibrada com sucesso.", COLOR_GREEN, COLOR_BLACK, 1);
    if (err == ESP_OK)
        display_draw_string(20, 68, "Salvo na memoria interna.", COLOR_CYAN, COLOR_BLACK, 1);
    display_draw_string(20, 100, "Iniciando em 3 segundos...", COLOR_GRAY, COLOR_BLACK, 1);

    vTaskDelay(pdMS_TO_TICKS(3000));
    return ESP_OK;
}

esp_err_t calibration_save(const touch_cal_t *cal)
{
    nvs_handle_t h;
    esp_err_t err = nvs_open(CAL_NVS_NAMESPACE, NVS_READWRITE, &h);
    if (err != ESP_OK)
        return err;

    // Salva como blob binário (6 floats = 24 bytes)
    err = nvs_set_blob(h, CAL_NVS_KEY, cal, sizeof(touch_cal_t));
    if (err == ESP_OK)
        nvs_commit(h);
    nvs_close(h);

    printf("[CAL] Calibracao salva no NVS: %s\n",
           err == ESP_OK ? "OK" : esp_err_to_name(err));
    return err;
}

esp_err_t calibration_load(touch_cal_t *cal)
{
    memset(cal, 0, sizeof(touch_cal_t));
    cal->valid = false;

    nvs_handle_t h;
    esp_err_t err = nvs_open(CAL_NVS_NAMESPACE, NVS_READONLY, &h);
    if (err != ESP_OK)
        return err;

    size_t size = sizeof(touch_cal_t);
    err = nvs_get_blob(h, CAL_NVS_KEY, cal, &size);
    nvs_close(h);

    if (err == ESP_OK && size == sizeof(touch_cal_t) && cal->valid)
    {
        printf("[CAL] Calibracao carregada do NVS\n");
        printf("[CAL] Coef X: ax=%.4f bx=%.4f cx=%.2f\n", cal->ax, cal->bx, cal->cx);
        printf("[CAL] Coef Y: ay=%.4f by=%.4f cy=%.2f\n", cal->ay, cal->by, cal->cy);
    }
    else
    {
        cal->valid = false;
    }

    return err;
}

void calibration_erase(void)
{
    nvs_handle_t h;
    if (nvs_open(CAL_NVS_NAMESPACE, NVS_READWRITE, &h) == ESP_OK)
    {
        nvs_erase_key(h, CAL_NVS_KEY);
        nvs_commit(h);
        nvs_close(h);
        printf("[CAL] Calibracao apagada do NVS\n");
    }
}

bool calibration_apply(const touch_cal_t *cal,
                       uint16_t rx, uint16_t ry,
                       int16_t *px, int16_t *py)
{
    if (!cal || !cal->valid)
        return false;

    float frx = (float)rx;
    float fry = (float)ry;

    int16_t x = (int16_t)(cal->ax * frx + cal->bx * fry + cal->cx);
    int16_t y = (int16_t)(cal->ay * frx + cal->by * fry + cal->cy);

    // Clamp dentro da tela
    if (x < 0)
        x = 0;
    if (x >= DISPLAY_WIDTH)
        x = DISPLAY_WIDTH - 1;
    if (y < 0)
        y = 0;
    if (y >= DISPLAY_HEIGHT)
        y = DISPLAY_HEIGHT - 1;

    *px = x;
    *py = y;
    return true;
}