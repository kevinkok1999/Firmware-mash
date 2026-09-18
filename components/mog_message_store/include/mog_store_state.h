#ifndef MOG_STORE_STATE_H
#define MOG_STORE_STATE_H

#include <stddef.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

typedef uint32_t (*mog_store_state_key_fn)(const void *record, void *ctx);

typedef enum {
    MOG_STATE_OK = 0,
    MOG_STATE_RECOVERED_PARTIAL = 1,
    MOG_STATE_ERR_ARG = -1,
    MOG_STATE_ERR_IO = -2,
    MOG_STATE_ERR_FORMAT = -3,
    MOG_STATE_ERR_FULL = -4,
    MOG_STATE_ERR_SEQUENCE = -5,
} mog_store_state_result_t;

typedef struct {
    uint8_t *records;
    uint32_t record_size;
    uint32_t capacity;
    uint32_t count;
    mog_store_state_key_fn key_fn;
    void *key_ctx;
} mog_store_state_t;

typedef struct {
    uint64_t snapshot_generation;
    uint64_t snapshot_watermark;
    uint64_t journal_max_sequence;
    uint64_t next_sequence;
    uint64_t next_generation;
    size_t journal_valid_bytes;
    int snapshot_present;
    int journal_recovered_partial;
} mog_store_recovery_info_t;

int mog_store_state_init(mog_store_state_t *state,
                         void *records,
                         uint32_t capacity,
                         uint32_t record_size,
                         mog_store_state_key_fn key_fn,
                         void *key_ctx);

void mog_store_state_clear(mog_store_state_t *state);

int mog_store_state_put(mog_store_state_t *state,
                        uint32_t key,
                        const void *record,
                        uint32_t record_size);

int mog_store_state_delete(mog_store_state_t *state, uint32_t key);

void mog_store_state_retain_recent(mog_store_state_t *state, uint32_t keep_count);

int mog_store_state_copy_recent(const mog_store_state_t *state,
                                void *out_records,
                                uint32_t max_records);

int mog_store_state_recover(mog_store_state_t *state,
                            const char *slot_a,
                            const char *slot_b,
                            const char *journal_path,
                            void *scratch,
                            size_t scratch_size,
                            mog_store_recovery_info_t *out_info);

int mog_store_state_checkpoint(mog_store_state_t *state,
                               const char *slot_a,
                               const char *slot_b,
                               const char *journal_path,
                               uint64_t current_generation,
                               uint64_t last_sequence,
                               uint64_t *out_generation);

#ifdef __cplusplus
}
#endif

#endif /* MOG_STORE_STATE_H */
