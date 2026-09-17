#ifndef TEST_BRAMBLE_MSG_STORE_SPIFFS_H
#define TEST_BRAMBLE_MSG_STORE_SPIFFS_H

#include "msg_store.h"

#include <stdbool.h>
#include <string.h>

static inline bool msg_store_record_matches(const stored_msg_t *a, const stored_msg_t *b) {
    if (!a || !b) {
        return false;
    }
    return a->uid == b->uid && a->peer_addr == b->peer_addr &&
           a->direction == b->direction && a->channel_index == b->channel_index &&
           a->text_len == b->text_len && memcmp(a->text, b->text, a->text_len) == 0;
}

int msg_store_spiffs_init(void);
int msg_store_spiffs_save(const stored_msg_t *msg);
int msg_store_spiffs_update(int from_end, const stored_msg_t *msg);
int msg_store_spiffs_get_count(void);
int msg_store_spiffs_load_recent(stored_msg_t *msgs, int max_count);
void msg_store_spiffs_rollover(int max_messages, int keep_pct);
void msg_store_spiffs_clear(void);

#endif
