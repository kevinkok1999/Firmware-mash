#ifndef MOG_STORE_JOURNAL_H
#define MOG_STORE_JOURNAL_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOG_STORE_JOURNAL_FORMAT_VERSION 1u

typedef enum {
    MOG_STORE_JOURNAL_PUT = 1,
    MOG_STORE_JOURNAL_DELETE = 2,
} mog_store_journal_op_t;

typedef enum {
    MOG_JOURNAL_OK = 0,
    MOG_JOURNAL_RECOVERED_PARTIAL = 1,
    MOG_JOURNAL_ERR_ARG = -1,
    MOG_JOURNAL_ERR_IO = -2,
    MOG_JOURNAL_ERR_FORMAT = -3,
    MOG_JOURNAL_ERR_CRC = -4,
    MOG_JOURNAL_ERR_SIZE = -5,
    MOG_JOURNAL_ERR_CALLBACK = -6,
} mog_store_journal_result_t;

typedef struct {
    uint64_t valid_entries;
    uint64_t max_sequence;
    size_t valid_bytes;
    int recovered_partial;
} mog_store_journal_scan_info_t;

typedef int (*mog_store_journal_replay_cb)(mog_store_journal_op_t op,
                                           uint64_t sequence,
                                           uint32_t uid,
                                           const void *payload,
                                           uint32_t payload_size,
                                           void *ctx);

/*
 * Append one durable journal operation.
 *
 * PUT requires payload_size == record_size and payload != NULL.
 * DELETE requires payload_size == 0 and payload == NULL.
 * sequence and uid must be non-zero.
 *
 * The commit marker is written only after the entry payload is fsync'd. A
 * reset before the second fsync leaves an invalid tail that replay ignores.
 */
int mog_store_journal_append(const char *path,
                             mog_store_journal_op_t op,
                             uint64_t sequence,
                             uint32_t uid,
                             const void *payload,
                             uint32_t payload_size,
                             uint32_t record_size);

/*
 * Replay the valid prefix of a journal in ascending sequence order.
 *
 * Entries with sequence <= min_sequence_exclusive are validated but not
 * delivered to the callback; this is how a committed snapshot watermark
 * prevents pre-snapshot journal entries from being applied twice.
 *
 * A torn/corrupt tail returns MOG_JOURNAL_RECOVERED_PARTIAL and reports the
 * number of bytes that were fully validated. The caller may then truncate the
 * journal to valid_bytes. Corruption never causes later bytes to be trusted.
 */
int mog_store_journal_replay(const char *path,
                             uint32_t record_size,
                             uint64_t min_sequence_exclusive,
                             void *scratch,
                             size_t scratch_size,
                             mog_store_journal_replay_cb callback,
                             void *ctx,
                             mog_store_journal_scan_info_t *out_info);

/* Truncate a journal to a previously validated prefix and fsync it. */
int mog_store_journal_truncate(const char *path, size_t valid_bytes);

#ifdef __cplusplus
}
#endif

#endif /* MOG_STORE_JOURNAL_H */
