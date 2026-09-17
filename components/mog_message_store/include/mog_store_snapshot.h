#ifndef MOG_STORE_SNAPSHOT_H
#define MOG_STORE_SNAPSHOT_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOG_STORE_SNAPSHOT_FORMAT_VERSION 1u

typedef enum {
    MOG_STORE_OK = 0,
    MOG_STORE_ERR_ARG = -1,
    MOG_STORE_ERR_IO = -2,
    MOG_STORE_ERR_FORMAT = -3,
    MOG_STORE_ERR_CRC = -4,
    MOG_STORE_ERR_SIZE = -5,
    MOG_STORE_ERR_NO_VALID_SLOT = -6,
} mog_store_result_t;

typedef struct {
    uint64_t generation;
    uint32_t record_count;
    uint32_t record_size;
    uint32_t payload_crc32;
} mog_store_snapshot_info_t;

/* IEEE CRC-32. `seed` is the previously returned CRC when processing data in chunks. */
uint32_t mog_store_crc32(const void *data, size_t len, uint32_t seed);

/*
 * Write one complete snapshot. Durability protocol:
 *   1. write an explicitly uncommitted header + payload;
 *   2. fflush/fsync the payload;
 *   3. write a self-validating committed header;
 *   4. fflush/fsync the commit header.
 *
 * The caller writes only to an inactive/shadow path. Existing committed state
 * must remain untouched until this function returns success.
 */
int mog_store_snapshot_write(const char *path,
                             uint64_t generation,
                             const void *records,
                             uint32_t record_count,
                             uint32_t record_size);

/* Validate magic/version/record size/file size/header CRC/payload CRC/commit marker. */
int mog_store_snapshot_validate(const char *path,
                                uint32_t expected_record_size,
                                mog_store_snapshot_info_t *out_info);

/*
 * Validate and read a complete committed snapshot.
 *
 * `records_capacity` is measured in records, not bytes. If the snapshot holds
 * more records than the supplied capacity, MOG_STORE_ERR_SIZE is returned and
 * no partial snapshot is exposed to the caller. Empty snapshots are valid and
 * may use records == NULL with records_capacity == 0.
 */
int mog_store_snapshot_read(const char *path,
                            uint32_t expected_record_size,
                            void *records,
                            uint32_t records_capacity,
                            mog_store_snapshot_info_t *out_info);

/* Pick the newest valid generation. If one slot is torn/corrupt, use the other. */
int mog_store_snapshot_select(const char *slot_a,
                              const char *slot_b,
                              uint32_t expected_record_size,
                              char *out_path,
                              size_t out_path_size,
                              mog_store_snapshot_info_t *out_info);

#ifdef __cplusplus
}
#endif

#endif /* MOG_STORE_SNAPSHOT_H */
