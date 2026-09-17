#ifndef TEST_ESP_SPIFFS_H
#define TEST_ESP_SPIFFS_H

#include <stdbool.h>

static inline bool esp_spiffs_mounted(const char *partition_label) {
    (void)partition_label;
    return true;
}

#endif
