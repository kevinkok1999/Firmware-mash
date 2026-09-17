#include "mog_store_snapshot.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

typedef struct {
    uint32_t id;
    char text[16];
} test_record_t;

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

static void truncate_file(const char *path, off_t size) {
    assert(truncate(path, size) == 0);
}

int main(void) {
    const char *slot_a = "/tmp/mog-store-a.bin";
    const char *slot_b = "/tmp/mog-store-b.bin";
    unlink(slot_a);
    unlink(slot_b);

    /* Known IEEE CRC-32 vector + incremental/chunked equivalence. */
    const char *vector = "123456789";
    const uint32_t crc_full = mog_store_crc32(vector, 9, 0);
    uint32_t crc_chunked = mog_store_crc32(vector, 4, 0);
    crc_chunked = mog_store_crc32(vector + 4, 5, crc_chunked);
    assert(crc_full == 0xcbf43926u);
    assert(crc_chunked == crc_full);

    const test_record_t generation_1[] = {
        {1, "one"},
        {2, "two"},
    };
    const test_record_t generation_2[] = {
        {2, "two"},
        {3, "three"},
    };

    assert(mog_store_snapshot_write(slot_a, 1, generation_1, 2, sizeof(test_record_t)) ==
           MOG_STORE_OK);

    char selected[64];
    mog_store_snapshot_info_t info;
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_a) == 0);
    assert(info.generation == 1);
    assert(info.record_count == 2);

    /* A fully committed newer slot must win. */
    assert(mog_store_snapshot_write(slot_b, 2, generation_2, 2, sizeof(test_record_t)) ==
           MOG_STORE_OK);
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_b) == 0);
    assert(info.generation == 2);

    /* Simulate a torn/incomplete newer generation: the older slot must remain usable. */
    truncate_file(slot_b, 24);
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_a) == 0);
    assert(info.generation == 1);

    /* Simulate payload corruption in a newer generation: fall back, never accept it. */
    assert(mog_store_snapshot_write(slot_b, 3, generation_2, 2, sizeof(test_record_t)) ==
           MOG_STORE_OK);
    corrupt_byte(slot_b, 48);
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_a) == 0);

    /* Wrong record schema is never interpreted as a valid snapshot. */
    assert(mog_store_snapshot_validate(slot_a, sizeof(test_record_t) + 1, NULL) ==
           MOG_STORE_ERR_FORMAT);

    unlink(slot_a);
    unlink(slot_b);
    puts("test_mog_store_snapshot: PASS");
    return 0;
}
