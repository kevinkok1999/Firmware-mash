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
    MOG_DEDUP_ERR_NOT_DURABLE = -4,
} mog_dedup_result_t;

typedef struct {
    mog_message_key_t key;
    uint32_t received_at_ms;
    bool durable;
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
 * authoritative MessageStore. Restored records are therefore durable. */
int mog_dedup_restore(mog_dedup_t *dedup, mog_message_key_t key,
                      bool delivered_to_chat, bool ack_required,
                      uint32_t received_at_ms);

/* Stage an authenticated logical message in RAM. A first receipt is NOT safe
 * to present or ACK yet: the caller must durably commit receiver truth in the
 * authoritative MessageStore, then call mog_dedup_mark_durable(). This closes
 * the persist -> present -> ACK crash window without giving dedup storage
 * authority. A durable duplicate may be presented only if it was not already
 * presented, and may always regenerate an ACK for lost-ACK recovery. */
int mog_dedup_receive(mog_dedup_t *dedup, mog_message_key_t key,
                      uint32_t now_ms, bool *should_present,
                      bool *should_ack);

/* Call only after MessageStore has committed the receiver record. Returns the
 * post-commit actions that are now safe. Repeating this call is idempotent. */
int mog_dedup_mark_durable(mog_dedup_t *dedup, mog_message_key_t key,
                           bool *should_present, bool *should_ack);

/* Presentation and ACK completion are rejected until durable receiver truth
 * exists. Presentation remains a separate step so a crash after persistence
 * but before UI delivery can be recovered deterministically. */
int mog_dedup_mark_presented(mog_dedup_t *dedup, mog_message_key_t key);
int mog_dedup_mark_ack_sent(mog_dedup_t *dedup, mog_message_key_t key);

const mog_dedup_entry_t *mog_dedup_find(const mog_dedup_t *dedup,
                                        mog_message_key_t key);

#ifdef __cplusplus
}
#endif
#endif
