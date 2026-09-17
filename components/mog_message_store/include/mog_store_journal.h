#ifndef MOG_STORE_JOURNAL_H
#define MOG_STORE_JOURNAL_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

#define MOG_STORE_JOURNAL_FORMAT_VERSION 1u

typedef enum {
    MOG_JOURNAL_OK = 0,
    MOG_JOURNAL_ERR_ARG = -1,
    MOG_JOURNAL_ERR_IO = -2,
    MOG_JOURNAL_ERR_FORMAT = -3,
    MOG_JOURNAL_ERR_CRC = -4,
    MOG_JOURNAL_ERR_SIZE = -5,
    MOG_JOURNAL_ERR_SEQUENCE = -6,
} mog_store_journal_result_t;

typedef enum {
    MOG_JOURNAL_OP_UPSERT = 1,
    MOG_JOURNAL_OP_DELETE = 2,
} mog_store_journal_op_t;

typedef struct {
    uint64_t sequence;
    uint32_t op;
    uint32_t key;
    const void *payload;
    uint32_t payload_len;
} mog_store_journal_entry_t;

typedef struct {
    uint64_t last_sequence;
    uint32_t entry_count;
    uint64_t valid_bytes;
    bool tail_damaged;
} mog_store_journal_scan_info_t;

typedef int (*mog_store_journal_replay_fn)(const mog_store_journal_entry_t *entry, void *ctx);

/*
 * Append one fully checksummed event and fsync it before returning success.
 * The caller owns sequence allocation. Sequence 0 is invalid.
 */
int mog_store_journal_append(const char *path,
                             uint64_t sequence,
                             uint32_t op,
                             uint32_t key,
                             const void *payload,
                             uint32_t payload_len);

/*
 * Replay valid entries in sequence order. A torn/corrupt trailing entry is not
 * exposed; replay stops at the last valid byte and reports tail_damaged=true.
 * Non-increasing sequence values are treated as corruption and are not replayed.
 */
int mog_store_journal_replay(const char *path,
                             uint32_t max_payload_len,
                             mog_store_journal_replay_fn callback,
                             void *ctx,
                             mog_store_journal_scan_info_t *out_info);

/*
 * Scan using the same rules as replay and truncate any invalid/torn suffix to
 * the last fully committed entry. This never invents an entry or skips over
 * corruption to accept later bytes.
 */
int mog_store_journal_repair_tail(const char *path,
                                  uint32_t max_payload_len,
                                  mog_store_journal_scan_info_t *out_info);

#ifdef __cplusplus
}
#endif

#endif /* MOG_STORE_JOURNAL_H */
