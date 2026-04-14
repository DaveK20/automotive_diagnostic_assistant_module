#ifndef SIMPLE_LOG_H
#define SIMPLE_LOG_H

#include <stdio.h>
#include "esp_timer.h"
#include "esp_system.h"
#include "esp_heap_caps.h"

// Níveis
#define LOG_LEVEL_INFO    1
#define LOG_LEVEL_WARN    2
#define LOG_LEVEL_ERROR   3

#ifndef LOG_LEVEL
#define LOG_LEVEL LOG_LEVEL_INFO
#endif

// Macro base
#define LOG_BASE(level, tag, fmt, ...) { \
    int64_t time_us = esp_timer_get_time(); \
    uint32_t ms = time_us / 1000; \
    uint32_t heap = heap_caps_get_free_size(MALLOC_CAP_8BIT); \
    int freq = CONFIG_ESP_DEFAULT_CPU_FREQ_MHZ; \
    printf("[%lu ms] [%s] [%s] [heap:%lu] [cpu:%dMHz] " fmt "\n", \
        ms, level, tag, heap, freq, ##__VA_ARGS__); \
}

// Interfaces
#define LOG_INFO(tag, fmt, ...) \
    if (LOG_LEVEL <= LOG_LEVEL_INFO) LOG_BASE("INFO", tag, fmt, ##__VA_ARGS__)

#define LOG_WARN(tag, fmt, ...) \
    if (LOG_LEVEL <= LOG_LEVEL_WARN) LOG_BASE("WARN", tag, fmt, ##__VA_ARGS__)

#define LOG_ERROR(tag, fmt, ...) \
    if (LOG_LEVEL <= LOG_LEVEL_ERROR) LOG_BASE("ERROR", tag, fmt, ##__VA_ARGS__)

#endif