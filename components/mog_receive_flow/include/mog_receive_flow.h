#ifndef MOG_RECEIVE_FLOW_H
#define MOG_RECEIVE_FLOW_H

#include <stdbool.h>
#include <stdint.h>

#include "mog_dedup.h"
#include "mog_receiver_store.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOG_RECEIVE_FLOW_OK = 0,
    MOG_RECEIVE_FLOW_DUPLICATE = 1,
    MOG_RECEIVE_FLOW_ERR_ARG = -1,
    MOG_RECEIVE_FLOW_ERR_STORE = -2,
    MOG_RECEIVE_FLOW_ERR_DEDUP = -3,
} mog_receive_flow_result_t;

/*
 * Rebuild the receiver RAM mirror exclusively from authoritative MessageStore
 * records after store recovery has completed. Reboot intentionally makes ACK
 * eligible again: a duplicate E2E ACK is safe and repairs the lost-ACK case.
 */
int mog_receive_flow_restore(mog_dedup_t *dedup,
                             const mog_receiver_store_t *store);

/*
 * Stage a logical receive, durably establish RECEIVED in MessageStore, then
 * release presentation/ACK actions through dedup. No action escapes before the
 * durable barrier. Equality is always the full (origin, PacketId) key.
 */
int mog_receive_flow_receive(mog_receiver_store_t *store,
                             mog_dedup_t *dedup,
                             mog_message_key_t key,
                             uint32_t now_ms,
                             bool *should_present,
                             bool *should_ack);

/*
 * Call after the UI has idempotently accepted the same logical key. Durable
 * PRESENTED truth is written first; only then is the RAM mirror strengthened.
 */
int mog_receive_flow_commit_presented(mog_receiver_store_t *store,
                                      mog_dedup_t *dedup,
                                      mog_message_key_t key);

#ifdef __cplusplus
}
#endif
#endif
