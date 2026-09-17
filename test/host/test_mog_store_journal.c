#include "mog_store_journal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    uint32_t seen;
    uint64_t last_sequence;
    uint32_t last_key;
    char last_payload[32];
} replay_state_t;

static int on_entry(const mog_store_journal_entry_t *entry, void *ctx) {
    replay_state_t *state = (replay_state_t *)ctx;
    assert(entry != NULL);
    assert(state != NULL);
    state->seen++;
    state->last_sequence = entry->sequence;
    state->last_key = entry->key;
    memset(state->last_payload, 0, sizeof(state->last_payload));
    if (entry->payload && entry->payload_len > 0) {
        const size_t n = entry->payload_len < sizeof(state->last_payload) - 1
                             ? entry->payload_len
                             : sizeof(state->last_payload) - 1;
        memcpy(state->last_payload, entry->payload, n);
    }
    return 0;
}

static void truncate_file(const char *path, off_t size) {
    assert(truncate(path, size) == 0);
}

static void corrupt_last_byte(const char *path) {
    FILE *f = fopen(path, "r+b");
    assert(f != NULL);
    assert(fseek(f, -1, SEEK_END) == 0);
    int byte = fgetc(f);
    assert(byte != EOF);
    assert(fseek(f, -1, SEEK_CUR) == 0);
    assert(fputc(byte ^ 0x7f, f) != EOF);
    assert(fflush(f) == 0);
    assert(fsync(fileno(f)) == 0);
    assert(fclose(f) == 0);
}

int main(void) {
    const char *path = "/tmp/mog-store-journal.bin";
    unlink(path);

    const char first[] = "message-one";
    const char second[] = "message-two";

    assert(mog_store_journal_append(path, 1, MOG_JOURNAL_OP_UPSERT, 11,
                                    first, sizeof(first)) == MOG_JOURNAL_OK);
    assert(mog_store_journal_append(path, 2, MOG_JOURNAL_OP_UPSERT, 22,
                                    second, sizeof(second)) == MOG_JOURNAL_OK);

    replay_state_t state = {0};
    mog_store_journal_scan_info_t info = {0};
    assert(mog_store_journal_replay(path, 128, on_entry, &state, &info) == MOG_JOURNAL_OK);
    assert(state.seen == 2);
    assert(state.last_sequence == 2);
    assert(state.last_key == 22);
    assert(strcmp(state.last_payload, second) == 0);
    assert(info.entry_count == 2);
    assert(info.last_sequence == 2);
    assert(!info.tail_damaged);

    /* Add a DELETE event with no payload. */
    assert(mog_store_journal_append(path, 3, MOG_JOURNAL_OP_DELETE, 11,
                                    NULL, 0) == MOG_JOURNAL_OK);
    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, 128, on_entry, &state, &info) == MOG_JOURNAL_OK);
    assert(state.seen == 3);
    assert(state.last_sequence == 3);
    assert(info.entry_count == 3);

    /* Simulate a torn fourth entry: replay must expose only the three good entries. */
    const long good_size = (long)info.valid_bytes;
    assert(mog_store_journal_append(path, 4, MOG_JOURNAL_OP_UPSERT, 44,
                                    second, sizeof(second)) == MOG_JOURNAL_OK);
    truncate_file(path, good_size + 10);

    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, 128, on_entry, &state, &info) == MOG_JOURNAL_OK);
    assert(state.seen == 3);
    assert(info.entry_count == 3);
    assert(info.last_sequence == 3);
    assert(info.tail_damaged);
    assert((long)info.valid_bytes == good_size);

    /* Repair truncates exactly to the last committed event. */
    assert(mog_store_journal_repair_tail(path, 128, &info) == MOG_JOURNAL_OK);
    assert(!info.tail_damaged);
    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, 128, on_entry, &state, &info) == MOG_JOURNAL_OK);
    assert(state.seen == 3);
    assert(!info.tail_damaged);

    /* Payload corruption of the last entry is treated as a damaged tail. */
    assert(mog_store_journal_append(path, 4, MOG_JOURNAL_OP_UPSERT, 44,
                                    second, sizeof(second)) == MOG_JOURNAL_OK);
    corrupt_last_byte(path);
    memset(&state, 0, sizeof(state));
    assert(mog_store_journal_replay(path, 128, on_entry, &state, &info) == MOG_JOURNAL_OK);
    assert(state.seen == 3);
    assert(info.tail_damaged);
    assert(info.last_sequence == 3);

    assert(mog_store_journal_repair_tail(path, 128, &info) == MOG_JOURNAL_OK);

    /* Invalid arguments are rejected rather than written. */
    assert(mog_store_journal_append(path, 0, MOG_JOURNAL_OP_UPSERT, 1,
                                    first, sizeof(first)) == MOG_JOURNAL_ERR_ARG);
    assert(mog_store_journal_append(path, 5, MOG_JOURNAL_OP_DELETE, 1,
                                    first, sizeof(first)) == MOG_JOURNAL_ERR_ARG);

    unlink(path);
    puts("test_mog_store_journal: PASS");
    return 0;
}
