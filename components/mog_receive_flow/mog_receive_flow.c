#include "mog_receive_flow.h"

static const mog_receiver_record_t *record_at(const mog_receiver_store_t *store,
                                               uint32_t index)
{
    return (const mog_receiver_record_t *)(store->state.records +
           ((size_t)index * store->state.record_size));
}

int mog_receive_flow_restore(mog_dedup_t *dedup,
                             const mog_receiver_store_t *store)
{
    if (dedup == NULL || store == NULL || store->state.records == NULL)
        return MOG_RECEIVE_FLOW_ERR_ARG;
    if (store->state.count > MOG_DEDUP_MAX_ENTRIES)
        return MOG_RECEIVE_FLOW_ERR_DEDUP;

    mog_dedup_init(dedup);
    for (uint32_t i = 0; i < store->state.count; ++i) {
        const mog_receiver_record_t *record = record_at(store, i);
        const mog_message_key_t key = {
            .origin = record->origin,
            .packet_id = record->packet_id,
        };
        const bool presented =
            (record->flags & MOG_RECEIVER_FLAG_PRESENTED) != 0;
        const int rc = mog_dedup_restore(dedup, key, presented, true, 0);
        if (rc != MOG_DEDUP_OK)
            return MOG_RECEIVE_FLOW_ERR_DEDUP;
    }
    return MOG_RECEIVE_FLOW_OK;
}

int mog_receive_flow_receive(mog_receiver_store_t *store,
                             mog_dedup_t *dedup,
                             mog_message_key_t key,
                             uint32_t now_ms,
                             bool *should_present,
                             bool *should_ack)
{
    if (store == NULL || dedup == NULL || should_present == NULL ||
        should_ack == NULL || !mog_message_key_is_valid(key))
        return MOG_RECEIVE_FLOW_ERR_ARG;

    *should_present = false;
    *should_ack = false;

    bool staged_present = false;
    bool staged_ack = false;
    const int dedup_rc = mog_dedup_receive(dedup, key, now_ms,
                                           &staged_present, &staged_ack);
    if (dedup_rc < 0)
        return MOG_RECEIVE_FLOW_ERR_DEDUP;

    const mog_dedup_entry_t *entry = mog_dedup_find(dedup, key);
    if (entry == NULL)
        return MOG_RECEIVE_FLOW_ERR_DEDUP;

    if (!entry->durable) {
        if (mog_receiver_store_mark_received(store, key) != MOG_RECEIVER_STORE_OK)
            return MOG_RECEIVE_FLOW_ERR_STORE;
        if (mog_dedup_mark_durable(dedup, key, should_present, should_ack) !=
            MOG_DEDUP_OK)
            return MOG_RECEIVE_FLOW_ERR_DEDUP;
    } else {
        *should_present = staged_present;
        *should_ack = staged_ack;
    }

    return dedup_rc == MOG_DEDUP_DUPLICATE ?
           MOG_RECEIVE_FLOW_DUPLICATE : MOG_RECEIVE_FLOW_OK;
}

int mog_receive_flow_commit_presented(mog_receiver_store_t *store,
                                      mog_dedup_t *dedup,
                                      mog_message_key_t key)
{
    if (store == NULL || dedup == NULL || !mog_message_key_is_valid(key))
        return MOG_RECEIVE_FLOW_ERR_ARG;

    if (mog_receiver_store_mark_presented(store, key) != MOG_RECEIVER_STORE_OK)
        return MOG_RECEIVE_FLOW_ERR_STORE;
    if (mog_dedup_mark_presented_durable(dedup, key) != MOG_DEDUP_OK)
        return MOG_RECEIVE_FLOW_ERR_DEDUP;
    return MOG_RECEIVE_FLOW_OK;
}
