#ifndef MOG_CONVERSATION_H
#define MOG_CONVERSATION_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mog_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef uint64_t mog_conversation_id_t;
typedef uint64_t mog_conversation_seq_t;

typedef struct {
    mog_conversation_id_t conversation_id;
    mog_message_key_t message;
    mog_conversation_seq_t sender_seq;
} mog_conversation_message_meta_t;

typedef struct {
    mog_message_key_t message;
    mog_conversation_seq_t sender_seq;
    bool occupied;
} mog_conversation_pending_t;

typedef enum {
    MOG_CONVERSATION_OK = 0,
    MOG_CONVERSATION_BUFFERED = 1,
    MOG_CONVERSATION_DUPLICATE = 2,
    MOG_CONVERSATION_ERR_ARG = -1,
    MOG_CONVERSATION_ERR_WINDOW = -2,
    MOG_CONVERSATION_ERR_FULL = -3,
    MOG_CONVERSATION_ERR_SEQUENCE_CONFLICT = -4,
    MOG_CONVERSATION_ERR_GAP_CONFLICT = -5,
} mog_conversation_result_t;

typedef struct {
    mog_conversation_id_t conversation_id;
    mog_node_id_t sender_origin;
    mog_conversation_seq_t last_presented_seq;
    bool has_checkpoint;
    mog_conversation_pending_t *pending;
    size_t pending_capacity;
    size_t pending_count;
    mog_conversation_seq_t reorder_window;
} mog_conversation_order_t;

/*
 * ConversationID is route/transport independent. The caller owns creation and
 * persistence of conversation identities; this module only enforces stable
 * identity/order semantics.
 *
 * sender_seq is NOT PacketId. sender_seq must be assigned transactionally by
 * the conversation/message creation layer so it stays monotonic for one sender
 * inside one conversation. PacketId remains the globally durable logical
 * message identifier together with its origin.
 */
int mog_conversation_order_init(mog_conversation_order_t *state,
                                mog_conversation_id_t conversation_id,
                                mog_node_id_t sender_origin,
                                mog_conversation_pending_t *pending_storage,
                                size_t pending_capacity,
                                mog_conversation_seq_t reorder_window,
                                bool has_checkpoint,
                                mog_conversation_seq_t last_presented_seq);

/*
 * Accept one message metadata record.
 *
 * ready_messages receives the logical message keys that may now be presented,
 * in sender order. To guarantee atomic state advancement, ready_capacity must
 * be at least pending_capacity + 1. No payload is stored here.
 */
int mog_conversation_accept(mog_conversation_order_t *state,
                            const mog_conversation_message_meta_t *meta,
                            mog_message_key_t *ready_messages,
                            size_t ready_capacity,
                            size_t *ready_count);

/*
 * Explicitly close a known missing sequence gap after Reliability/TTL policy
 * has decided those sequence numbers will not arrive. This never happens just
 * because a timer wrapped or a transport changed. Any buffered message at or
 * below through_seq makes the call fail rather than silently discard data.
 */
int mog_conversation_advance_gap(mog_conversation_order_t *state,
                                 mog_conversation_seq_t through_seq,
                                 mog_message_key_t *ready_messages,
                                 size_t ready_capacity,
                                 size_t *ready_count);

bool mog_conversation_id_is_valid(mog_conversation_id_t id);
bool mog_conversation_same_chat(mog_conversation_id_t a, mog_conversation_id_t b);

#ifdef __cplusplus
}
#endif

#endif /* MOG_CONVERSATION_H */
