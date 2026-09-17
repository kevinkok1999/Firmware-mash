#include "mog_store_state.h"
#include "mog_store_journal.h"
#include "mog_store_snapshot.h"

#include <limits.h>
#include <string.h>

static void *record_at(mog_store_state_t *state, uint32_t index) {
    return state->records + ((size_t)index * state->record_size);
}

static const void *record_at_const(const mog_store_state_t *state, uint32_t index) {
    return state->records + ((size_t)index * state->record_size);
}

static int find_key(const mog_store_state_t *state, uint32_t key) {
    for (uint32_t i = 0; i < state->count; ++i) {
        if (state->key_fn(record_at_const(state, i), state->key_ctx) == key) {
            return (int)i;
        }
    }
    return -1;
}

int mog_store_state_init(mog_store_state_t *state,
                         void *records,
                         uint32_t capacity,
                         uint32_t record_size,
                         mog_store_state_key_fn key_fn,
                         void *key_ctx) {
    if (!state || !records || capacity == 0 || record_size == 0 || !key_fn) {
        return MOG_STATE_ERR_ARG;
    }
    state->records = (uint8_t *)records;
    state->record_size = record_size;
    state->capacity = capacity;
    state->count = 0;
    state->key_fn = key_fn;
    state->key_ctx = key_ctx;
    return MOG_STATE_OK;
}

void mog_store_state_clear(mog_store_state_t *state) {
    if (state) {
        state->count = 0;
    }
}

int mog_store_state_put(mog_store_state_t *state,
                        uint32_t key,
                        const void *record,
                        uint32_t record_size) {
    if (!state || !record || key == 0 || record_size != state->record_size) {
        return MOG_STATE_ERR_ARG;
    }
    if (state->key_fn(record, state->key_ctx) != key) {
        return MOG_STATE_ERR_FORMAT;
    }

    const int existing = find_key(state, key);
    if (existing >= 0) {
        memcpy(record_at(state, (uint32_t)existing), record, state->record_size);
        return MOG_STATE_OK;
    }
    if (state->count >= state->capacity) {
        return MOG_STATE_ERR_FULL;
    }
    memcpy(record_at(state, state->count), record, state->record_size);
    state->count++;
    return MOG_STATE_OK;
}

int mog_store_state_delete(mog_store_state_t *state, uint32_t key) {
    if (!state || key == 0) {
        return MOG_STATE_ERR_ARG;
    }
    const int existing = find_key(state, key);
    if (existing < 0) {
        return MOG_STATE_OK;
    }
    const uint32_t idx = (uint32_t)existing;
    if (idx + 1 < state->count) {
        memmove(record_at(state, idx), record_at(state, idx + 1),
                (size_t)(state->count - idx - 1) * state->record_size);
    }
    state->count--;
    return MOG_STATE_OK;
}

void mog_store_state_retain_recent(mog_store_state_t *state, uint32_t keep_count) {
    if (!state || keep_count >= state->count) {
        return;
    }
    if (keep_count == 0) {
        state->count = 0;
        return;
    }
    const uint32_t skip = state->count - keep_count;
    memmove(state->records, record_at(state, skip),
            (size_t)keep_count * state->record_size);
    state->count = keep_count;
}

int mog_store_state_copy_recent(const mog_store_state_t *state,
                                void *out_records,
                                uint32_t max_records) {
    if (!state || (!out_records && max_records > 0)) {
        return MOG_STATE_ERR_ARG;
    }
    const uint32_t count = state->count < max_records ? state->count : max_records;
    if (count == 0) {
        return 0;
    }
    const uint32_t skip = state->count - count;
    memcpy(out_records, record_at_const(state, skip),
           (size_t)count * state->record_size);
    return (int)count;
}

typedef struct {
    mog_store_state_t *state;
    int apply_error;
} replay_ctx_t;

static int replay_entry(mog_store_journal_op_t op,
                        uint64_t sequence,
                        uint32_t uid,
                        const void *payload,
                        uint32_t payload_size,
                        void *ctx) {
    (void)sequence;
    replay_ctx_t *replay = (replay_ctx_t *)ctx;
    int rc;
    if (op == MOG_STORE_JOURNAL_PUT) {
        rc = mog_store_state_put(replay->state, uid, payload, payload_size);
    } else {
        rc = mog_store_state_delete(replay->state, uid);
    }
    if (rc != MOG_STATE_OK) {
        replay->apply_error = rc;
        return -1;
    }
    return 0;
}

static int load_snapshot_if_present(mog_store_state_t *state,
                                    const char *slot_a,
                                    const char *slot_b,
                                    mog_store_snapshot_info_t *snapshot_info,
                                    int *snapshot_present) {
    char selected[256];
    const int select_rc = mog_store_snapshot_select(slot_a, slot_b, state->record_size,
                                                    selected, sizeof(selected), snapshot_info);
    if (select_rc == MOG_STORE_ERR_NO_VALID_SLOT) {
        *snapshot_present = 0;
        memset(snapshot_info, 0, sizeof(*snapshot_info));
        return MOG_STATE_OK;
    }
    if (select_rc != MOG_STORE_OK) {
        return MOG_STATE_ERR_IO;
    }
    if (snapshot_info->record_count > state->capacity) {
        return MOG_STATE_ERR_FULL;
    }
    const int read_rc = mog_store_snapshot_read(selected, state->record_size, state->records,
                                                state->capacity, snapshot_info);
    if (read_rc != MOG_STORE_OK) {
        return MOG_STATE_ERR_FORMAT;
    }
    state->count = snapshot_info->record_count;

    for (uint32_t i = 0; i < state->count; ++i) {
        const uint32_t key = state->key_fn(record_at_const(state, i), state->key_ctx);
        if (key == 0) {
            return MOG_STATE_ERR_FORMAT;
        }
        for (uint32_t j = 0; j < i; ++j) {
            if (state->key_fn(record_at_const(state, j), state->key_ctx) == key) {
                return MOG_STATE_ERR_FORMAT;
            }
        }
    }
    *snapshot_present = 1;
    return MOG_STATE_OK;
}

int mog_store_state_recover(mog_store_state_t *state,
                            const char *slot_a,
                            const char *slot_b,
                            const char *journal_path,
                            void *scratch,
                            size_t scratch_size,
                            mog_store_recovery_info_t *out_info) {
    if (!state || !slot_a || !slot_b || !journal_path || !scratch ||
        scratch_size < state->record_size) {
        return MOG_STATE_ERR_ARG;
    }
    state->count = 0;

    mog_store_snapshot_info_t snapshot = {0};
    int snapshot_present = 0;
    int rc = load_snapshot_if_present(state, slot_a, slot_b, &snapshot, &snapshot_present);
    if (rc != MOG_STATE_OK) {
        return rc;
    }

    replay_ctx_t replay = {.state = state, .apply_error = MOG_STATE_OK};
    mog_store_journal_scan_info_t journal = {0};
    const int journal_rc = mog_store_journal_replay(
        journal_path, state->record_size, snapshot.last_sequence, scratch, scratch_size,
        replay_entry, &replay, &journal);
    if (replay.apply_error != MOG_STATE_OK) {
        return replay.apply_error;
    }
    if (journal_rc < 0) {
        return MOG_STATE_ERR_IO;
    }

    const uint64_t max_sequence = journal.max_sequence > snapshot.last_sequence
                                      ? journal.max_sequence : snapshot.last_sequence;
    if (max_sequence == UINT64_MAX || snapshot.generation == UINT64_MAX) {
        return MOG_STATE_ERR_SEQUENCE;
    }

    if (out_info) {
        out_info->snapshot_generation = snapshot.generation;
        out_info->snapshot_watermark = snapshot.last_sequence;
        out_info->journal_max_sequence = journal.max_sequence;
        out_info->next_sequence = max_sequence + 1;
        out_info->next_generation = snapshot_present ? snapshot.generation + 1 : 1;
        out_info->journal_valid_bytes = journal.valid_bytes;
        out_info->snapshot_present = snapshot_present;
        out_info->journal_recovered_partial = journal.recovered_partial;
    }

    return journal_rc == MOG_JOURNAL_RECOVERED_PARTIAL
               ? MOG_STATE_RECOVERED_PARTIAL : MOG_STATE_OK;
}

int mog_store_state_checkpoint(mog_store_state_t *state,
                               const char *slot_a,
                               const char *slot_b,
                               const char *journal_path,
                               uint64_t current_generation,
                               uint64_t last_sequence,
                               uint64_t *out_generation) {
    if (!state || !slot_a || !slot_b || !journal_path || current_generation == UINT64_MAX) {
        return MOG_STATE_ERR_ARG;
    }

    char selected[256];
    mog_store_snapshot_info_t info = {0};
    const int select_rc = mog_store_snapshot_select(slot_a, slot_b, state->record_size,
                                                    selected, sizeof(selected), &info);
    const char *inactive = slot_a;
    if (select_rc == MOG_STORE_OK) {
        inactive = strcmp(selected, slot_a) == 0 ? slot_b : slot_a;
    } else if (select_rc != MOG_STORE_ERR_NO_VALID_SLOT) {
        return MOG_STATE_ERR_IO;
    }

    const uint64_t new_generation = current_generation + 1;
    if (mog_store_snapshot_write(inactive, new_generation, last_sequence,
                                 state->records, state->count,
                                 state->record_size) != MOG_STORE_OK) {
        return MOG_STATE_ERR_IO;
    }

    mog_store_snapshot_info_t verify = {0};
    if (mog_store_snapshot_validate(inactive, state->record_size, &verify) != MOG_STORE_OK ||
        verify.generation != new_generation || verify.last_sequence != last_sequence ||
        verify.record_count != state->count) {
        return MOG_STATE_ERR_FORMAT;
    }

    if (mog_store_journal_truncate(journal_path, 0) != MOG_JOURNAL_OK) {
        return MOG_STATE_ERR_IO;
    }
    if (out_generation) {
        *out_generation = new_generation;
    }
    return MOG_STATE_OK;
}
