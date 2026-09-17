#ifndef TEST_ESP_LOG_H
#define TEST_ESP_LOG_H

#include <stdio.h>

#define ESP_LOGI(tag, fmt, ...) \
    do { (void)(tag); fprintf(stderr, "I: " fmt "\n", ##__VA_ARGS__); } while (0)
#define ESP_LOGW(tag, fmt, ...) \
    do { (void)(tag); fprintf(stderr, "W: " fmt "\n", ##__VA_ARGS__); } while (0)
#define ESP_LOGE(tag, fmt, ...) \
    do { (void)(tag); fprintf(stderr, "E: " fmt "\n", ##__VA_ARGS__); } while (0)

#endif
