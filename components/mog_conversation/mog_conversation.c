#include "mog_conversation.h"

#include <limits.h>

static bool meta_matches_state(const mog_conversation_order_t *state,
                               const mog_conversation_message_meta_t *meta) {
    return state && meta &&
           meta->conversation_id == state->conversation_id &&
           meta->message.origin == state->sender_origin &&
           mog_message_key_is_valid(meta->message) &&
           meta->sender_seq != 0;
}

static size_t find_pending_seq(const mog_conversation_order_t *state,
                               mog_conversation_seq_t seq) {
    if (!state || !state->pending) {
        return SIZE_MAX;
    }
    for (size_t i = 0; i < state->pending_capacity; ++i) {
        if (state->pending[i].occupied && state->pending[i].sender_seq == seq) {
            return i;
        }
    }
    return SIZE_MAX;
}

static size_t find_free_slot(const mog_conversation_order_t *state) {
    if (!state || !state->pending) {
        return SIZE_MAX;
    }
    for (size_t i = 0; i < state->pending_capacity; ++i) {
        if (!state->pending[i].occupied) {
            return i;
        }
    }
    return SIZE_MAX;
}

static int flush_contiguous(mog_conversation_order_t *state,
                            mog_message_key_t *ready_messages,
                            size_t ready_capacity,
                            size_t *ready_count) {
    if (!state || !ready_messages || !ready_count) {
        return MOG_CONVERSATION_ERR_ARG;
    }

    while (state->has_checkpoint && state->last_presented_seq != UINT64_MAX) {
        const mog_conversation_seq_t wanted = state->last_presented_seq + 1;
        const size_t idx = find_pending_seq(state, wanted);
        if (idx == SIZE_MAX) {
            break;
        }
        if (*ready_count >= ready_capacity) {
            return MOG_CONVERSATION_ERR_ARG;
        }

        ready_messages[*ready_count] = state->pending[idx].message;
        (*ready_count)++;
        state->pending[idx].occupied = false;
        state->pending_count--;
        state->last_presented_seq = wanted;
    }
    return MOG_CONVERSATION_OK;
}

int mog_conversation_order_init(mog_conversation_order_t *state,
                                mog_conversation_id_t conversation_id,
                                mog_node_id_t sender_origin,
                                mog_conversation_pending_t *pending_storage,
                                size_t pending_capacity,
                                mog_conversation_seq_t reorder_window,
                                bool has_checkpoint,
                                mog_conversation_seq_t last_presented_seq) {
    if (!state || !mog_conversation_id_is_valid(conversation_id) || sender_origin == 0 ||
        !pending_storage || pending_capacity == 0 || reorder_window == 0 ||
        (has_checkpoint && last_presented_seq == 0)) {
        return MOG_CONVERSATION_ERR_ARG;
    }

    state->conversation_id = conversation_id;
    state->sender_origin = sender_origin;
    state->last_presented_seq = has_checkpoint ? last_presented_seq : 0;
    state->has_checkpoint = has_checkpoint;
    state->pending = pending_storage;
    state->pending_capacity = pending_capacity;
    state->pending_count = 0;
    state->reorder_window = reorder_window;

    for (size_t i = 0; i < pending_capacity; ++i) {
        state->pending[i].occupied = false;
        state->pending[i].sender_seq = 0;
        state->pending[i].message = (mog_message_key_t){0};
    }
    return MOG_CONVERSATION_OK;
}

int mog_conversation_accept(mog_conversation_order_t *state,
                            const mog_conversation_message_meta_t *meta,
                            mog_message_key_t *ready_messages,
                            size_t ready_capacity,
                            size_t *ready_count) {
    if (!state || !meta || !ready_messages || !ready_count ||
        ready_capacity < state->pending_capacity + 1 || !meta_matches_state(state, meta)) {
        return MOG_CONVERSATION_ERR_ARG;
    }
    *ready_count = 0;

    if (!state->has_checkpoint) {
        state->has_checkpoint = true;
        state->last_presented_seq = meta->sender_seq;
        ready_messages[0] = meta->message;
        *ready_count = 1;
        return MOG_CONVERSATION_OK;
    }

    if (meta->sender_seq <= state->last_presented_seq) {
        return MOG_CONVERSATION_DUPLICATE;
    }

    const mog_conversation_seq_t delta = meta->sender_seq - state->last_presented_seq;
    if (delta > state->reorder_window) {
        return MOG_CONVERSATION_ERR_WINDOW;
    }

    if (delta == 1) {
        ready_messages[0] = meta->message;
        *ready_count = 1;
        state->last_presented_seq = meta->sender_seq;
        return flush_contiguous(state, ready_messages, ready_capacity, ready_count);
    }

    const size_t existing = find_pending_seq(state, meta->sender_seq);
    if (existing != SIZE_MAX) {
        if (mog_message_key_equal(state->pending[existing].message, meta->message)) {
            return MOG_CONVERSATION_DUPLICATE;
        }
        return MOG_CONVERSATION_ERR_SEQUENCE_CONFLICT;
    }

    if (state->pending_count >= state->pending_capacity) {
        return MOG_CONVERSATION_ERR_FULL;
    }
    const size_t free_idx = find_free_slot(state);
    if (free_idx == SIZE_MAX) {
        return MOG_CONVERSATION_ERR_FULL;
    }

    state->pending[free_idx].message = meta->message;
    state->pending[free_idx].sender_seq = meta->sender_seq;
    state->pending[free_idx].occupied = true;
    state->pending_count++;
    return MOG_CONVERSATION_BUFFERED;
}

int mog_conversation_advance_gap(mog_conversation_order_t *state,
                                 mog_conversation_seq_t through_seq,
                                 mog_message_key_t *ready_messages,
                                 size_t ready_capacity,
                                 size_t *ready_count) {
    if (!state || !ready_messages || !ready_count || !state->has_checkpoint ||
        through_seq <= state->last_presented_seq ||
        ready_capacity < state->pending_capacity + 1) {
        return MOG_CONVERSATION_ERR_ARG;
    }
    *ready_count = 0;

    for (size_t i = 0; i < state->pending_capacity; ++i) {
        if (state->pending[i].occupied && state->pending[i].sender_seq <= through_seq) {
            return MOG_CONVERSATION_ERR_GAP_CONFLICT;
        }
    }

    state->last_presented_seq = through_seq;
    return flush_contiguous(state, ready_messages, ready_capacity, ready_count);
}

bool mog_conversation_id_is_valid(mog_conversation_id_t id) {
    return id != 0;
}

bool mog_conversation_same_chat(mog_conversation_id_t a, mog_conversation_id_t b) {
    return a != 0 && a == b;
}
