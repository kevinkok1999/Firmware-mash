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
 * the receive -> persist barrier without giving dedup storage authority. A
 * durable duplicate may be presented only if durable MessageStore truth does
 * not already say it was presented, and may regenerate an ACK for lost-ACK
 * recovery. */
int mog_dedup_receive(mog_dedup_t *dedup, mog_message_key_t key,
                      uint32_t now_ms, bool *should_present,
                      bool *should_ack);

/* Call only after MessageStore has committed the receiver record. Returns the
 * post-commit actions that are now safe. Repeating this call is idempotent. */
int mog_dedup_mark_durable(mog_dedup_t *dedup, mog_message_key_t key,
                           bool *should_present, bool *should_ack);

/* Commit presentation truth into the RAM mirror only AFTER the authoritative
 * MessageStore has durably recorded delivered_to_chat=true for this key. This
 * ordering is deliberate: marking RAM first could suppress a message after a
 * reboot even though durable truth still says it needs presentation. The UI
 * layer must itself be idempotent by the same (origin, PacketId) key across the
 * store-commit -> UI-render crash window; dedup does not claim impossible
 * transactional exactly-once side effects across storage and UI. */
int mog_dedup_mark_presented_durable(mog_dedup_t *dedup,
                                     mog_message_key_t key);

/* ACK completion need not be durable for correctness: a reboot may regenerate
 * an end-to-end ACK for a duplicate, which is explicitly safe and bounded. */
int mog_dedup_mark_ack_sent(mog_dedup_t *dedup, mog_message_key_t key);

const mog_dedup_entry_t *mog_dedup_find(const mog_dedup_t *dedup,
                                        mog_message_key_t key);

#ifdef __cplusplus
}
#endif
#endif
