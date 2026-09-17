#ifndef MOG_RELIABILITY_H
#define MOG_RELIABILITY_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mog_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOG_RELIABILITY_MAX_TRACKED 32u

typedef enum {
    MOG_REL_OK = 0,
    MOG_REL_ERR_ARG = -1,
    MOG_REL_ERR_FULL = -2,
    MOG_REL_ERR_NOT_FOUND = -3,
    MOG_REL_ERR_STATE = -4,
} mog_reliability_result_t;

typedef struct {
    mog_message_key_t key;
    mog_message_state_t state;
    uint32_t next_retry_ms;
    uint32_t retry_base_ms;
    uint8_t attempts;
    uint8_t max_attempts;
    bool in_use;
} mog_reliability_entry_t;

typedef struct {
    mog_reliability_entry_t entries[MOG_RELIABILITY_MAX_TRACKED];
    size_t count;
} mog_reliability_t;

void mog_reliability_init(mog_reliability_t *rel);
int mog_reliability_track(mog_reliability_t *rel, mog_message_key_t key,
                          uint8_t max_attempts, uint32_t retry_base_ms);
int mog_reliability_note_send(mog_reliability_t *rel, mog_message_key_t key,
                              uint32_t now_ms);
int mog_reliability_note_link_success(mog_reliability_t *rel,
                                      mog_message_key_t key);
int mog_reliability_note_e2e_ack(mog_reliability_t *rel,
                                 mog_message_key_t key);
int mog_reliability_defer_no_route(mog_reliability_t *rel,
                                   mog_message_key_t key);
int mog_reliability_note_route_available(mog_reliability_t *rel,
                                         mog_message_key_t key);
int mog_reliability_due(const mog_reliability_t *rel, uint32_t now_ms,
                        mog_message_key_t *out, size_t out_capacity,
                        size_t *out_count);
int mog_reliability_sweep_exhausted(mog_reliability_t *rel, uint32_t now_ms,
                                    size_t *failed_count);
const mog_reliability_entry_t *mog_reliability_find(const mog_reliability_t *rel,
                                                     mog_message_key_t key);

#ifdef __cplusplus
}
#endif
#endif
