#ifndef MOG_DEDUP_H
#define MOG_DEDUP_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include "mog_core.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOG_DEDUP_MAX_ENTRIES 64u

typedef enum {
    MOG_DEDUP_OK = 0,
    MOG_DEDUP_DUPLICATE = 1,
    MOG_DEDUP_ERR_ARG = -1,
    MOG_DEDUP_ERR_FULL = -2,
    MOG_DEDUP_ERR_NOT_FOUND = -3,
} mog_dedup_result_t;

typedef struct {
    mog_message_key_t key;
    uint32_t received_at_ms;
    bool delivered_to_chat;
    bool ack_required;
    bool in_use;
} mog_dedup_entry_t;

typedef struct {
    mog_dedup_entry_t entries[MOG_DEDUP_MAX_ENTRIES];
    size_t count;
} mog_dedup_t;

void mog_dedup_init(mog_dedup_t *dedup);

/* Restore receiver-side truth only from a record already committed by the
 * authoritative MessageStore. This component deliberately owns no storage. */
int mog_dedup_restore(mog_dedup_t *dedup, mog_message_key_t key,
                      bool delivered_to_chat, bool ack_required,
                      uint32_t received_at_ms);

/* Record a newly authenticated logical message. A duplicate never creates a
 * second chat item; duplicate reception may still require an ACK resend. */
int mog_dedup_receive(mog_dedup_t *dedup, mog_message_key_t key,
                      uint32_t now_ms, bool *should_present,
                      bool *should_ack);

/* Mark presentation only after the authoritative store has durably committed
 * the receiver-side delivery/presentation record. */
int mog_dedup_mark_presented(mog_dedup_t *dedup, mog_message_key_t key);

/* ACK transmission is transport work, not user-visible delivery truth. */
int mog_dedup_mark_ack_sent(mog_dedup_t *dedup, mog_message_key_t key);

const mog_dedup_entry_t *mog_dedup_find(const mog_dedup_t *dedup,
                                        mog_message_key_t key);

#ifdef __cplusplus
}
#endif
#endif
