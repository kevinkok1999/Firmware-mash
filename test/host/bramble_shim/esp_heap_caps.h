#ifndef TEST_ESP_HEAP_CAPS_H
#define TEST_ESP_HEAP_CAPS_H

#include <stddef.h>
#include <stdlib.h>

#define MALLOC_CAP_SPIRAM 1
#define MALLOC_CAP_DEFAULT 2

static inline void *heap_caps_calloc(size_t n, size_t size, int caps) {
    (void)caps;
    return calloc(n, size);
}

static inline void heap_caps_free(void *ptr) {
    free(ptr);
}

#endif
