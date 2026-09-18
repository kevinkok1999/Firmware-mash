#ifndef MOG_RECEIVER_STORE_H
#define MOG_RECEIVER_STORE_H

#include <stdbool.h>
#include <stdint.h>

#include "mog_core.h"
#include "mog_store_state.h"

#ifdef __cplusplus
extern "C" {
#endif

#define MOG_RECEIVER_STORE_SCHEMA_VERSION 1u
#define MOG_RECEIVER_FLAG_RECEIVED  (1u << 0)
#define MOG_RECEIVER_FLAG_PRESENTED (1u << 1)

typedef struct {
    uint32_t uid; /* journal-local stable id; NOT logical message identity */
    uint16_t schema_version;
    uint16_t flags;
    mog_node_id_t origin;
    mog_packet_id_t packet_id;
} mog_receiver_record_t;

typedef struct {
    mog_store_state_t state;
    const char *journal_path;
    uint64_t next_sequence;
    uint32_t next_uid;
} mog_receiver_store_t;

typedef enum {
    MOG_RECEIVER_STORE_OK = 0,
    MOG_RECEIVER_STORE_ERR_ARG = -1,
    MOG_RECEIVER_STORE_ERR_IO = -2,
    MOG_RECEIVER_STORE_ERR_FULL = -3,
    MOG_RECEIVER_STORE_ERR_SEQUENCE = -4,
    MOG_RECEIVER_STORE_ERR_FORMAT = -5,
} mog_receiver_store_result_t;

/*
 * Receiver records live inside the authoritative MessageStore. The uid only
 * addresses journal mutations. Equality/dedup is ALWAYS the full
 * origin-qualified (origin, packet_id) key; callers must never hash/truncate
 * that logical identity into uid.
 */
int mog_receiver_store_init(mog_receiver_store_t *store,
                            mog_receiver_record_t *records,
                            uint32_t capacity,
                            const char *journal_path,
                            uint64_t next_sequence);

/* Re-establish uid/sequence allocators after mog_store_state_recover(). */
int mog_receiver_store_finish_recovery(mog_receiver_store_t *store,
                                       uint64_t next_sequence);

const mog_receiver_record_t *mog_receiver_store_find(const mog_receiver_store_t *store,
                                                      mog_message_key_t key);

/* Durable append happens before RAM truth changes. Existing keys are idempotent. */
int mog_receiver_store_mark_received(mog_receiver_store_t *store,
                                     mog_message_key_t key);
int mog_receiver_store_mark_presented(mog_receiver_store_t *store,
                                      mog_message_key_t key);

#ifdef __cplusplus
}
#endif

#endif /* MOG_RECEIVER_STORE_H */
