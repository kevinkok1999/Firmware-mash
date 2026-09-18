#include "mog_store_journal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    uint32_t id;
    char text[16];
} test_record_t;

typedef struct {
    unsigned calls;
    uint64_t last_sequence;
    uint32_t last_uid;
    mog_store_journal_op_t last_op;
    test_record_t last_record;
} replay_state_t;

static int on_replay(mog_store_journal_op_t op,
                     uint64_t sequence,
                     uint32_t uid,
                     const void *payload,
                     uint32_t payload_size,
                     void *ctx) {
    replay_state_t *state = (replay_state_t *)ctx;
    state->calls++;
    state->last_sequence = sequence;
    state->last_uid = uid;
    state->last_op = op;
    if (op == MOG_STORE_JOURNAL_PUT) {
        assert(payload != NULL);
        assert(payload_size == sizeof(test_record_t));
        memcpy(&state->last_record, payload, sizeof(test_record_t));
    } else {
        assert(payload == NULL);
        assert(payload_size == 0);
    }
    return 0;
}

static void corrupt_byte(const char *path, long offset) {
    FILE *f = fopen(path, "r+b");
    assert(f != NULL);
    assert(fseek(f, offset, SEEK_SET) == 0);
    const int byte = fgetc(f);
    assert(byte != EOF);
    assert(fseek(f, offset, SEEK_SET) == 0);
    assert(fputc(byte ^ 0x5a, f) != EOF);
    assert(fflush(f) == 0);
    assert(fsync(fileno(f)) == 0);
    assert(fclose(f) == 0);
}

int main(void) {
    const char *path = "/tmp/mog-store-journal.bin";
    unlink(path);

    const test_record_t a = {1, "alpha"};
    const test_record_t b = {2, "beta"};

    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_PUT, 1, 100, &a,
                                    sizeof(a), sizeof(a)) == MOG_JOURNAL_OK);
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_PUT, 2, 101, &b,
                                    sizeof(b), sizeof(b)) == MOG_JOURNAL_OK);
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_DELETE, 3, 100, NULL,
                                    0, sizeof(a)) == MOG_JOURNAL_OK);

    test_record_t scratch;
    replay_state_t state = {0};
    mog_store_journal_scan_info_t info;
    assert(mog_store_journal_replay(path, sizeof(a), 0, &scratch, sizeof(scratch),
                                    on_replay, &state, &info) == MOG_JOURNAL_OK);
    assert(state.calls == 3);
    assert(state.last_sequence == 3);
    assert(state.last_uid == 100);
    assert(state.last_op == MOG_STORE_JOURNAL_DELETE);
    assert(info.valid_entries == 3);
    assert(info.max_sequence == 3);
    assert(!info.recovered_partial);

    /* Snapshot watermark semantics: validate old entries, replay only newer. */
    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, sizeof(a), 1, &scratch, sizeof(scratch),
                                    on_replay, &state, &info) == MOG_JOURNAL_OK);
    assert(state.calls == 2);
    assert(state.last_sequence == 3);

    /* A torn tail preserves the complete prefix and reports its safe truncation point. */
    const size_t safe_bytes = info.valid_bytes;
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_PUT, 4, 102, &a,
                                    sizeof(a), sizeof(a)) == MOG_JOURNAL_OK);
    FILE *f = fopen(path, "r+b");
    assert(f != NULL);
    assert(fseek(f, -5, SEEK_END) == 0);
    const long torn_size = ftell(f);
    assert(torn_size > 0);
    assert(fclose(f) == 0);
    assert(truncate(path, torn_size) == 0);

    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, sizeof(a), 0, &scratch, sizeof(scratch),
                                    on_replay, &state, &info) == MOG_JOURNAL_RECOVERED_PARTIAL);
    assert(state.calls == 3);
    assert(info.valid_entries == 3);
    assert(info.max_sequence == 3);
    assert(info.recovered_partial);
    assert(info.valid_bytes == safe_bytes);

    /* Boot repair must happen before another append; after truncation sequence 4 is reachable. */
    assert(mog_store_journal_truncate(path, info.valid_bytes) == MOG_JOURNAL_OK);
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_PUT, 4, 102, &a,
                                    sizeof(a), sizeof(a)) == MOG_JOURNAL_OK);
    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, sizeof(a), 0, &scratch, sizeof(scratch),
                                    on_replay, &state, &info) == MOG_JOURNAL_OK);
    assert(state.calls == 4);
    assert(state.last_sequence == 4);
    assert(state.last_uid == 102);
    assert(info.max_sequence == 4);
    assert(!info.recovered_partial);

    /* Restore the three-entry prefix for the CRC-tail test. */
    assert(mog_store_journal_truncate(path, safe_bytes) == MOG_JOURNAL_OK);

    /* A CRC-corrupted last committed entry is rejected; earlier state survives. */
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_PUT, 4, 102, &a,
                                    sizeof(a), sizeof(a)) == MOG_JOURNAL_OK);
    corrupt_byte(path, (long)safe_bytes + 40 + 1);
    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, sizeof(a), 0, &scratch, sizeof(scratch),
                                    on_replay, &state, &info) == MOG_JOURNAL_RECOVERED_PARTIAL);
    assert(state.calls == 3);
    assert(info.max_sequence == 3);
    assert(info.valid_bytes == safe_bytes);

    /* Invalid append contracts fail closed. */
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_PUT, 0, 1, &a,
                                    sizeof(a), sizeof(a)) == MOG_JOURNAL_ERR_ARG);
    assert(mog_store_journal_append(path, MOG_STORE_JOURNAL_DELETE, 5, 1, &a,
                                    sizeof(a), sizeof(a)) == MOG_JOURNAL_ERR_ARG);

    unlink(path);
    puts("test_mog_store_journal: PASS");
    return 0;
}
