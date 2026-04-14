#ifndef CALIBRATION_H
#define CALIBRATION_H

#include <stdint.h>
#include <stdbool.h>
#include "esp_err.h"

// ---------------------------------------------------------------
// Coeficientes da transformação afim 3-pontos
//
// A transformação afim mapeia coordenadas brutas (rx, ry) do ADC
// para pixels (px, py) na tela:
//
//   px = ax * rx + bx * ry + cx
//   py = ay * rx + by * ry + cy
//
// Isso compensa escala, offset E rotação/inclinação do painel
// resistivo em relação ao display — muito mais preciso que
// simplesmente mapear min/max de cada eixo separadamente.
// ---------------------------------------------------------------
typedef struct
{
    float ax, bx, cx; // coeficientes para eixo X
    float ay, by, cy; // coeficientes para eixo Y
    bool valid;       // true = calibração válida
} touch_cal_t;

// ---------------------------------------------------------------
// Executa a rotina de calibração interativa na tela.
// O usuário deve pressionar 3 alvos exibidos na tela.
// Ao final, salva os coeficientes em NVS automaticamente.
// ---------------------------------------------------------------
esp_err_t calibration_run(touch_cal_t *cal);

// ---------------------------------------------------------------
// Salva / carrega calibração no NVS
// ---------------------------------------------------------------
esp_err_t calibration_save(const touch_cal_t *cal);
esp_err_t calibration_load(touch_cal_t *cal);
void calibration_erase(void);

// ---------------------------------------------------------------
// Aplica a calibração: converte raw → pixel
// Retorna false se a calibração não for válida.
// ---------------------------------------------------------------
bool calibration_apply(const touch_cal_t *cal,
                       uint16_t rx, uint16_t ry,
                       int16_t *px, int16_t *py);

#endif // CALIBRATION_H