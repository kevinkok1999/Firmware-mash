#include "mog_reliability.h"
#include <limits.h>
#include <string.h>

static mog_reliability_entry_t *find_mut(mog_reliability_t *rel,
                                         mog_message_key_t key)
{
    size_t i;
    if (rel == NULL || !mog_message_key_is_valid(key)) return NULL;
    for (i = 0; i < MOG_RELIABILITY_MAX_TRACKED; ++i) {
        if (rel->entries[i].in_use &&
            mog_message_key_equal(rel->entries[i].key, key)) return &rel->entries[i];
    }
    return NULL;
}

static int transition(mog_reliability_entry_t *e, mog_message_state_t to)
{
    if (e == NULL || !mog_message_state_can_transition(e->state, to))
        return MOG_REL_ERR_STATE;
    e->state = to;
    return MOG_REL_OK;
}

void mog_reliability_init(mog_reliability_t *rel)
{
    if (rel != NULL) memset(rel, 0, sizeof(*rel));
}

const mog_reliability_entry_t *mog_reliability_find(const mog_reliability_t *rel,
                                                     mog_message_key_t key)
{
    size_t i;
    if (rel == NULL || !mog_message_key_is_valid(key)) return NULL;
    for (i = 0; i < MOG_RELIABILITY_MAX_TRACKED; ++i) {
        if (rel->entries[i].in_use &&
            mog_message_key_equal(rel->entries[i].key, key)) return &rel->entries[i];
    }
    return NULL;
}

int mog_reliability_track(mog_reliability_t *rel, mog_message_key_t key,
                          uint8_t max_attempts, uint32_t retry_base_ms)
{
    size_t i;
    /* mog_time_reached32() is only unambiguous for deadlines less than half
     * the uint32_t time space away. Reject larger retry bases up front so all
     * derived deadlines preserve that invariant. */
    if (rel == NULL || !mog_message_key_is_valid(key) || max_attempts == 0u ||
        retry_base_ms == 0u || retry_base_ms > (uint32_t)INT32_MAX)
        return MOG_REL_ERR_ARG;
    if (find_mut(rel, key) != NULL) return MOG_REL_OK;
    if (rel->count >= MOG_RELIABILITY_MAX_TRACKED) return MOG_REL_ERR_FULL;
    for (i = 0; i < MOG_RELIABILITY_MAX_TRACKED; ++i) {
        mog_reliability_entry_t *e = &rel->entries[i];
        if (!e->in_use) {
            memset(e, 0, sizeof(*e));
            e->key = key;
            e->state = MOG_MSG_READY;
            e->max_attempts = max_attempts;
            e->retry_base_ms = retry_base_ms;
            e->in_use = true;
            rel->count++;
            return MOG_REL_OK;
        }
    }
    return MOG_REL_ERR_FULL;
}

int mog_reliability_note_send(mog_reliability_t *rel, mog_message_key_t key,
                              uint32_t now_ms)
{
    mog_reliability_entry_t *e = find_mut(rel, key);
    uint32_t shift, delay;
    if (e == NULL) return MOG_REL_ERR_NOT_FOUND;
    if (e->attempts >= e->max_attempts) {
        if (transition(e, MOG_MSG_FAILED_PERMANENT) != MOG_REL_OK)
            return MOG_REL_ERR_STATE;
        return MOG_REL_ERR_STATE;
    }
    if (transition(e, MOG_MSG_SENDING) != MOG_REL_OK ||
        transition(e, MOG_MSG_WAITING_ACK) != MOG_REL_OK)
        return MOG_REL_ERR_STATE;
    e->attempts++;
    shift = e->attempts > 1u ? (uint32_t)e->attempts - 1u : 0u;
    if (shift > 6u) shift = 6u;
    if (e->retry_base_ms > ((uint32_t)INT32_MAX >> shift))
        delay = (uint32_t)INT32_MAX;
    else
        delay = e->retry_base_ms << shift;
    e->next_retry_ms = now_ms + delay;
    return MOG_REL_OK;
}

int mog_reliability_note_link_success(mog_reliability_t *rel,
                                      mog_message_key_t key)
{
    mog_reliability_entry_t *e = find_mut(rel, key);
    if (e == NULL) return MOG_REL_ERR_NOT_FOUND;
    if (mog_message_state_is_terminal(e->state)) return MOG_REL_ERR_STATE;
    return MOG_REL_OK; /* Link success is never end-to-end delivery truth. */
}

int mog_reliability_note_e2e_ack(mog_reliability_t *rel, mog_message_key_t key)
{
    mog_reliability_entry_t *e = find_mut(rel, key);
    if (e == NULL) return MOG_REL_ERR_NOT_FOUND;
    if (e->state == MOG_MSG_DELIVERED) return MOG_REL_OK;
    return transition(e, MOG_MSG_DELIVERED);
}

int mog_reliability_defer_no_route(mog_reliability_t *rel, mog_message_key_t key)
{
    mog_reliability_entry_t *e = find_mut(rel, key);
    int rc;
    if (e == NULL) return MOG_REL_ERR_NOT_FOUND;
    rc = transition(e, MOG_MSG_WAITING_ROUTE);
    if (rc != MOG_REL_OK) return rc;
    e->next_retry_ms = 0u;
    return MOG_REL_OK;
}

int mog_reliability_due(const mog_reliability_t *rel, uint32_t now_ms,
                        mog_message_key_t *out, size_t out_capacity,
                        size_t *out_count)
{
    size_t i, n = 0;
    if (rel == NULL || out_count == NULL || (out_capacity > 0u && out == NULL))
        return MOG_REL_ERR_ARG;
    for (i = 0; i < MOG_RELIABILITY_MAX_TRACKED; ++i) {
        const mog_reliability_entry_t *e = &rel->entries[i];
        if (!e->in_use || e->state != MOG_MSG_WAITING_ACK ||
            !mog_time_reached32(now_ms, e->next_retry_ms) ||
            e->attempts >= e->max_attempts) continue;
        if (n < out_capacity) out[n] = e->key;
        n++;
    }
    *out_count = n;
    return n > out_capacity ? MOG_REL_ERR_FULL : MOG_REL_OK;
}
