#include "mog_receiver_store.h"
#include "mog_store_journal.h"

#include <limits.h>
#include <string.h>

static uint32_t receiver_uid(const void *record, void *ctx) {
    (void)ctx;
    return ((const mog_receiver_record_t *)record)->uid;
}

static bool record_key_equal(const mog_receiver_record_t *record, mog_message_key_t key) {
    return record->origin == key.origin && record->packet_id == key.packet_id;
}

static mog_receiver_record_t *find_mut(mog_receiver_store_t *store, mog_message_key_t key) {
    for (uint32_t i = 0; i < store->state.count; ++i) {
        mog_receiver_record_t *record =
            (mog_receiver_record_t *)(store->state.records +
            ((size_t)i * store->state.record_size));
        if (record_key_equal(record, key)) {
            return record;
        }
    }
    return NULL;
}

static int validate_recovered_records(mog_receiver_store_t *store, uint32_t *max_uid) {
    *max_uid = 0;
    for (uint32_t i = 0; i < store->state.count; ++i) {
        const mog_receiver_record_t *record =
            (const mog_receiver_record_t *)(store->state.records +
            ((size_t)i * store->state.record_size));
        const mog_message_key_t key = {record->origin, record->packet_id};
        if (record->uid == 0 || record->schema_version != MOG_RECEIVER_STORE_SCHEMA_VERSION ||
            !mog_message_key_is_valid(key) ||
            (record->flags & MOG_RECEIVER_FLAG_RECEIVED) == 0 ||
            (record->flags & ~(MOG_RECEIVER_FLAG_RECEIVED | MOG_RECEIVER_FLAG_PRESENTED)) != 0) {
            return MOG_RECEIVER_STORE_ERR_FORMAT;
        }
        for (uint32_t j = 0; j < i; ++j) {
            const mog_receiver_record_t *prior =
                (const mog_receiver_record_t *)(store->state.records +
                ((size_t)j * store->state.record_size));
            if (record_key_equal(prior, key)) {
                return MOG_RECEIVER_STORE_ERR_FORMAT;
            }
        }
        if (record->uid > *max_uid) {
            *max_uid = record->uid;
        }
    }
    return MOG_RECEIVER_STORE_OK;
}

int mog_receiver_store_init(mog_receiver_store_t *store,
                            mog_receiver_record_t *records,
                            uint32_t capacity,
                            const char *journal_path,
                            uint64_t next_sequence) {
    if (!store || !records || capacity == 0 || !journal_path || next_sequence == 0) {
        return MOG_RECEIVER_STORE_ERR_ARG;
    }
    memset(store, 0, sizeof(*store));
    if (mog_store_state_init(&store->state, records, capacity, sizeof(*records),
                             receiver_uid, NULL) != MOG_STATE_OK) {
        return MOG_RECEIVER_STORE_ERR_ARG;
    }
    store->journal_path = journal_path;
    store->next_sequence = next_sequence;
    store->next_uid = 1;
    return MOG_RECEIVER_STORE_OK;
}

int mog_receiver_store_finish_recovery(mog_receiver_store_t *store,
                                       uint64_t next_sequence) {
    if (!store || next_sequence == 0) {
        return MOG_RECEIVER_STORE_ERR_ARG;
    }
    uint32_t max_uid = 0;
    const int rc = validate_recovered_records(store, &max_uid);
    if (rc != MOG_RECEIVER_STORE_OK) {
        return rc;
    }
    if (max_uid == UINT32_MAX || next_sequence == UINT64_MAX) {
        return MOG_RECEIVER_STORE_ERR_SEQUENCE;
    }
    store->next_uid = max_uid + 1;
    store->next_sequence = next_sequence;
    return MOG_RECEIVER_STORE_OK;
}

const mog_receiver_record_t *mog_receiver_store_find(const mog_receiver_store_t *store,
                                                      mog_message_key_t key) {
    if (!store || !mog_message_key_is_valid(key)) {
        return NULL;
    }
    for (uint32_t i = 0; i < store->state.count; ++i) {
        const mog_receiver_record_t *record =
            (const mog_receiver_record_t *)(store->state.records +
            ((size_t)i * store->state.record_size));
        if (record_key_equal(record, key)) {
            return record;
        }
    }
    return NULL;
}

static int durable_put(mog_receiver_store_t *store, const mog_receiver_record_t *record) {
    if (store->next_sequence == 0 || store->next_sequence == UINT64_MAX) {
        return MOG_RECEIVER_STORE_ERR_SEQUENCE;
    }
    if (mog_store_journal_append(store->journal_path, MOG_STORE_JOURNAL_PUT,
                                 store->next_sequence, record->uid, record,
                                 sizeof(*record), sizeof(*record)) != MOG_JOURNAL_OK) {
        return MOG_RECEIVER_STORE_ERR_IO;
    }
    if (mog_store_state_put(&store->state, record->uid, record,
                            sizeof(*record)) != MOG_STATE_OK) {
        /* Durable journal truth exists: fail closed instead of claiming success. */
        return MOG_RECEIVER_STORE_ERR_FORMAT;
    }
    store->next_sequence++;
    return MOG_RECEIVER_STORE_OK;
}

int mog_receiver_store_mark_received(mog_receiver_store_t *store,
                                     mog_message_key_t key) {
    if (!store || !mog_message_key_is_valid(key)) {
        return MOG_RECEIVER_STORE_ERR_ARG;
    }
    if (find_mut(store, key)) {
        return MOG_RECEIVER_STORE_OK;
    }
    if (store->state.count >= store->state.capacity) {
        return MOG_RECEIVER_STORE_ERR_FULL;
    }
    if (store->next_uid == 0 || store->next_uid == UINT32_MAX) {
        return MOG_RECEIVER_STORE_ERR_SEQUENCE;
    }
    mog_receiver_record_t record = {
        .uid = store->next_uid,
        .schema_version = MOG_RECEIVER_STORE_SCHEMA_VERSION,
        .flags = MOG_RECEIVER_FLAG_RECEIVED,
        .origin = key.origin,
        .packet_id = key.packet_id,
    };
    const int rc = durable_put(store, &record);
    if (rc == MOG_RECEIVER_STORE_OK) {
        store->next_uid++;
    }
    return rc;
}

int mog_receiver_store_mark_presented(mog_receiver_store_t *store,
                                      mog_message_key_t key) {
    if (!store || !mog_message_key_is_valid(key)) {
        return MOG_RECEIVER_STORE_ERR_ARG;
    }
    mog_receiver_record_t *existing = find_mut(store, key);
    if (!existing) {
        return MOG_RECEIVER_STORE_ERR_FORMAT;
    }
    if ((existing->flags & MOG_RECEIVER_FLAG_PRESENTED) != 0) {
        return MOG_RECEIVER_STORE_OK;
    }
    mog_receiver_record_t updated = *existing;
    updated.flags |= MOG_RECEIVER_FLAG_PRESENTED;
    return durable_put(store, &updated);
}
