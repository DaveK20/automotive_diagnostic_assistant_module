#ifndef OBD_H
#define OBD_H

#include <stdint.h>

uint16_t obd_get_rpm();

#endif

/*
coleta inicial 

RPM
Velocidade
Temperatura do motor
Nivel de combustivel
Posicao do acelerador
Tempo de motor ligado
*/


// possiveis dados a serem coletados
/*

Dados essenciais do veículo

Velocidade do veículo	0x0D	estatísticas e análise de condução
RPM do motor	0x0C	carga do motor
Temperatura do motor	0x05	detectar superaquecimento
Tempo de motor ligado	0x1F	tempo de uso
Carga do motor	0x04	eficiência e esforço do motor

Dados importantes para manutenção

Temperatura do óleo	0x5C	monitorar desgaste do motor
Nível de combustível	0x2F	gestão de consumo
Pressão do combustível	0x0A	diagnóstico
Temperatura do ar de admissão	0x0F	eficiência do motor

Dados para análise de condução

Posição do acelerador	0x11	agressividade na condução
Fluxo de ar MAF	0x10	consumo e eficiência
Pressão do coletor (MAP)	0x0B	carga do motor
Avanço da ignição	0x0E	desempenho

Dados de diagnóstico

Status do sistema de combustível	0x03
Fuel trim curto	0x06
Fuel trim longo	0x07
Códigos de erro (DTC)	Mode 03

*/