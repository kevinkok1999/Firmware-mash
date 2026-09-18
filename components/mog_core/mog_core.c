#include "mog_core.h"

#include <limits.h>
#include <string.h>

static int reserve_next_window(mog_packet_id_generator_t *gen) {
    if (!gen || !gen->write || gen->reserve == 0) {
        return MOG_CORE_ERR_ARG;
    }
    if (gen->durable_ceiling > UINT64_MAX - gen->reserve) {
        return MOG_CORE_ERR_EXHAUSTED;
    }

    const uint64_t new_ceiling = gen->durable_ceiling + gen->reserve;
    if (new_ceiling == 0 || gen->write(new_ceiling, gen->ctx) != 0) {
        return MOG_CORE_ERR_STORE;
    }

    gen->next = gen->durable_ceiling == 0 ? 1 : gen->durable_ceiling;
    gen->durable_ceiling = new_ceiling;
    gen->ready = true;
    return MOG_CORE_OK;
}

int mog_packet_id_generator_init(mog_packet_id_generator_t *gen,
                                 uint64_t reserve,
                                 mog_counter_read_fn read_fn,
                                 mog_counter_write_fn write_fn,
                                 void *ctx) {
    if (!gen || reserve < 2 || !read_fn || !write_fn) {
        return MOG_CORE_ERR_ARG;
    }

    /* Fail closed even when a caller reuses an old stack/static object. If
     * persistence read/reserve fails below, a subsequent next() cannot issue
     * from stale generator state left by an earlier initialization. */
    memset(gen, 0, sizeof(*gen));
    gen->reserve = reserve;
    gen->read = read_fn;
    gen->write = write_fn;
    gen->ctx = ctx;

    uint64_t old_ceiling = 0;
    if (read_fn(&old_ceiling, ctx) != 0) {
        return MOG_CORE_ERR_STORE;
    }
    if (old_ceiling == UINT64_MAX) {
        return MOG_CORE_ERR_EXHAUSTED;
    }

    gen->durable_ceiling = old_ceiling;
    return reserve_next_window(gen);
}

int mog_packet_id_next(mog_packet_id_generator_t *gen, mog_packet_id_t *out) {
    if (!gen || !out) {
        return MOG_CORE_ERR_ARG;
    }
    if (!gen->ready) {
        return MOG_CORE_ERR_STATE;
    }
    if (gen->next == 0) {
        return MOG_CORE_ERR_STATE;
    }

    if (gen->next >= gen->durable_ceiling) {
        const uint64_t previous_next = gen->next;
        const uint64_t previous_ceiling = gen->durable_ceiling;
        const bool previous_ready = gen->ready;

        const int rc = reserve_next_window(gen);
        if (rc != MOG_CORE_OK) {
            gen->next = previous_next;
            gen->durable_ceiling = previous_ceiling;
            gen->ready = previous_ready;
            return rc;
        }
    }

    if (gen->next == 0 || gen->next == UINT64_MAX) {
        return MOG_CORE_ERR_EXHAUSTED;
    }

    *out = gen->next;
    gen->next++;
    return MOG_CORE_OK;
}

bool mog_message_key_is_valid(mog_message_key_t key) {
    return key.origin != 0 && key.packet_id != 0;
}

bool mog_message_key_equal(mog_message_key_t a, mog_message_key_t b) {
    return a.origin == b.origin && a.packet_id == b.packet_id;
}

bool mog_message_state_is_terminal(mog_message_state_t state) {
    return state == MOG_MSG_DELIVERED || state == MOG_MSG_EXPIRED ||
           state == MOG_MSG_FAILED_PERMANENT;
}

bool mog_message_state_can_transition(mog_message_state_t from,
                                      mog_message_state_t to) {
    if (from == to) {
        return true;
    }
    if (mog_message_state_is_terminal(from)) {
        return false;
    }

    switch (from) {
        case MOG_MSG_CREATED:
            return to == MOG_MSG_READY || to == MOG_MSG_FAILED_PERMANENT;
        case MOG_MSG_READY:
            return to == MOG_MSG_SENDING || to == MOG_MSG_WAITING_ROUTE ||
                   to == MOG_MSG_DEFERRED || to == MOG_MSG_EXPIRED ||
                   to == MOG_MSG_FAILED_PERMANENT;
        case MOG_MSG_SENDING:
            return to == MOG_MSG_WAITING_ACK || to == MOG_MSG_WAITING_ROUTE ||
                   to == MOG_MSG_DEFERRED || to == MOG_MSG_DELIVERED ||
                   to == MOG_MSG_FAILED_PERMANENT;
        case MOG_MSG_WAITING_ACK:
            return to == MOG_MSG_SENDING || to == MOG_MSG_WAITING_ROUTE ||
                   to == MOG_MSG_DEFERRED || to == MOG_MSG_DELIVERED ||
                   to == MOG_MSG_EXPIRED || to == MOG_MSG_FAILED_PERMANENT;
        case MOG_MSG_WAITING_ROUTE:
            return to == MOG_MSG_READY || to == MOG_MSG_SENDING ||
                   to == MOG_MSG_DEFERRED || to == MOG_MSG_DELIVERED ||
                   to == MOG_MSG_EXPIRED || to == MOG_MSG_FAILED_PERMANENT;
        case MOG_MSG_DEFERRED:
            return to == MOG_MSG_READY || to == MOG_MSG_SENDING ||
                   to == MOG_MSG_WAITING_ROUTE || to == MOG_MSG_DELIVERED ||
                   to == MOG_MSG_EXPIRED || to == MOG_MSG_FAILED_PERMANENT;
        case MOG_MSG_DELIVERED:
        case MOG_MSG_EXPIRED:
        case MOG_MSG_FAILED_PERMANENT:
            return false;
    }
    return false;
}

uint32_t mog_time_elapsed32(uint32_t now, uint32_t then) {
    return now - then;
}

bool mog_time_reached32(uint32_t now, uint32_t deadline) {
    return (int32_t)(now - deadline) >= 0;
}
