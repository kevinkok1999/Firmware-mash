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

int main(void) {
    const char *slot_a = "/tmp/mog-store-a.bin";
    const char *slot_b = "/tmp/mog-store-b.bin";
    unlink(slot_a);
    unlink(slot_b);

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

    assert(mog_store_snapshot_write(slot_a, 1, 3, generation_1, 2,
                                    sizeof(test_record_t)) == MOG_STORE_OK);

    char selected[64];
    mog_store_snapshot_info_t info;
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_a) == 0);
    assert(info.generation == 1);
    assert(info.last_sequence == 3);
    assert(info.record_count == 2);

    test_record_t recovered[2] = {0};
    assert(mog_store_snapshot_read(slot_a, sizeof(test_record_t), recovered, 2,
                                   &info) == MOG_STORE_OK);
    assert(info.last_sequence == 3);
    assert(memcmp(recovered, generation_1, sizeof(generation_1)) == 0);

    test_record_t sentinel = {99, "unchanged"};
    assert(mog_store_snapshot_read(slot_a, sizeof(test_record_t), &sentinel, 1, NULL) ==
           MOG_STORE_ERR_SIZE);
    assert(sentinel.id == 99);
    assert(strcmp(sentinel.text, "unchanged") == 0);

    assert(mog_store_snapshot_write(slot_b, 2, 6, generation_2, 2,
                                    sizeof(test_record_t)) == MOG_STORE_OK);
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_b) == 0);
    assert(info.generation == 2);
    assert(info.last_sequence == 6);

    assert(truncate(slot_b, 24) == 0);
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_a) == 0);
    assert(info.last_sequence == 3);

    assert(mog_store_snapshot_write(slot_b, 3, 7, generation_2, 2,
                                    sizeof(test_record_t)) == MOG_STORE_OK);
    corrupt_byte(slot_b, 49);
    assert(mog_store_snapshot_select(slot_a, slot_b, sizeof(test_record_t), selected,
                                     sizeof(selected), &info) == MOG_STORE_OK);
    assert(strcmp(selected, slot_a) == 0);
    assert(mog_store_snapshot_read(slot_b, sizeof(test_record_t), recovered, 2, NULL) ==
           MOG_STORE_ERR_CRC);

    assert(mog_store_snapshot_validate(slot_a, sizeof(test_record_t) + 1, NULL) ==
           MOG_STORE_ERR_FORMAT);

    assert(mog_store_snapshot_write(slot_b, 4, 8, NULL, 0,
                                    sizeof(test_record_t)) == MOG_STORE_OK);
    assert(mog_store_snapshot_read(slot_b, sizeof(test_record_t), NULL, 0,
                                   &info) == MOG_STORE_OK);
    assert(info.generation == 4);
    assert(info.last_sequence == 8);
    assert(info.record_count == 0);

    unlink(slot_a);
    unlink(slot_b);
    puts("test_mog_store_snapshot: PASS");
    return 0;
}
