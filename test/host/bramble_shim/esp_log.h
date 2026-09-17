#ifndef TEST_ESP_LOG_H
#define TEST_ESP_LOG_H

#include <stdarg.h>
#include <stdio.h>

static inline void test_esp_log(const char *level, const char *fmt, ...) {
    va_list args;
    fprintf(stderr, "%s: ", level);
    va_start(args, fmt);
    vfprintf(stderr, fmt, args);
    va_end(args);
    fputc('\n', stderr);
}

#define ESP_LOGI(tag, ...) \
    do { (void)(tag); test_esp_log("I", __VA_ARGS__); } while (0)
#define ESP_LOGW(tag, ...) \
    do { (void)(tag); test_esp_log("W", __VA_ARGS__); } while (0)
#define ESP_LOGE(tag, ...) \
    do { (void)(tag); test_esp_log("E", __VA_ARGS__); } while (0)

#endif
