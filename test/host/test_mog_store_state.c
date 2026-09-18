#include "mog_store_state.h"
#include "mog_store_journal.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    uint32_t uid;
    uint32_t status;
    char text[16];
} rec_t;

static uint32_t key_of(const void *record, void *ctx) {
    (void)ctx;
    return ((const rec_t *)record)->uid;
}

int main(void) {
    const char *slot_a = "/tmp/mog-state-a.bin";
    const char *slot_b = "/tmp/mog-state-b.bin";
    const char *journal = "/tmp/mog-state-j.bin";
    unlink(slot_a);
    unlink(slot_b);
    unlink(journal);

    rec_t records[8];
    rec_t scratch;
    mog_store_state_t state;
    assert(mog_store_state_init(&state, records, 8, sizeof(rec_t), key_of, NULL) ==
           MOG_STATE_OK);

    mog_store_recovery_info_t info;
    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_OK);
    assert(state.count == 0);
    assert(info.next_sequence == 1);
    assert(info.next_generation == 1);

    /* Existing but invalid snapshot data is corruption, not a fresh empty store. */
    FILE *bad = fopen(slot_a, "wb");
    assert(bad != NULL);
    assert(fwrite("bad", 1, 3, bad) == 3);
    assert(fclose(bad) == 0);
    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_ERR_FORMAT);
    unlink(slot_a);

    rec_t r1 = {1, 1, "one"};
    rec_t r2 = {2, 1, "two"};
    assert(mog_store_journal_append(journal, MOG_STORE_JOURNAL_PUT, 1, 1, &r1,
                                    sizeof(r1), sizeof(r1)) == MOG_JOURNAL_OK);
    assert(mog_store_journal_append(journal, MOG_STORE_JOURNAL_PUT, 2, 2, &r2,
                                    sizeof(r2), sizeof(r2)) == MOG_JOURNAL_OK);
    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_OK);
    assert(state.count == 2);
    assert(info.next_sequence == 3);
    assert(info.next_generation == 1);

    uint64_t generation = 0;
    assert(mog_store_state_checkpoint(&state, slot_a, slot_b, journal, 0, 2,
                                      &generation) == MOG_STATE_OK);
    assert(generation == 1);
    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_OK);
    assert(state.count == 2);
    assert(info.snapshot_present);
    assert(info.snapshot_watermark == 2);
    assert(info.next_sequence == 3);

    r2.status = 7;
    assert(mog_store_journal_append(journal, MOG_STORE_JOURNAL_PUT, 3, 2, &r2,
                                    sizeof(r2), sizeof(r2)) == MOG_JOURNAL_OK);
    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_OK);
    assert(state.count == 2);
    assert(((rec_t *)state.records)[1].status == 7);
    assert(info.next_sequence == 4);

    assert(mog_store_journal_append(journal, MOG_STORE_JOURNAL_DELETE, 4, 1, NULL,
                                    0, sizeof(r1)) == MOG_JOURNAL_OK);
    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_OK);
    assert(state.count == 1);
    assert(((rec_t *)state.records)[0].uid == 2);
    assert(info.next_sequence == 5);

    rec_t r3 = {3, 1, "three"};
    assert(mog_store_journal_append(journal, MOG_STORE_JOURNAL_PUT, 5, 3, &r3,
                                    sizeof(r3), sizeof(r3)) == MOG_JOURNAL_OK);
    FILE *f = fopen(journal, "r+b");
    assert(f != NULL);
    assert(fseek(f, -3, SEEK_END) == 0);
    const long cut = ftell(f);
    assert(cut > 0);
    assert(fclose(f) == 0);
    assert(truncate(journal, cut) == 0);

    assert(mog_store_state_recover(&state, slot_a, slot_b, journal, &scratch,
                                   sizeof(scratch), &info) == MOG_STATE_RECOVERED_PARTIAL);
    assert(state.count == 1);
    assert(((rec_t *)state.records)[0].uid == 2);
    assert(info.journal_recovered_partial);
    assert(mog_store_journal_truncate(journal, info.journal_valid_bytes) == MOG_JOURNAL_OK);

    r3.uid = 0;
    assert(mog_store_state_put(&state, 3, &r3, sizeof(r3)) == MOG_STATE_ERR_FORMAT);

    unlink(slot_a);
    unlink(slot_b);
    unlink(journal);
    puts("test_mog_store_state: PASS");
    return 0;
}
