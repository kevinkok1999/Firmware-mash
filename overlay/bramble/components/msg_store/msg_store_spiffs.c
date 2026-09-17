#include "msg_store_spiffs.h"
#include "msg_store.h"
#include "mog_store_journal.h"
#include "mog_store_state.h"

#include <inttypes.h>
#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#ifdef ESP_PLATFORM

#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include "sdkconfig.h"

#define TAG "mog_msg_store"

#define MOG_SLOT_A_PATH "/spiffs/mog-msg-a.bin"
#define MOG_SLOT_B_PATH "/spiffs/mog-msg-b.bin"
#define MOG_JOURNAL_PATH "/spiffs/mog-msg-journal.bin"
#define LEGACY_MSG_PATH "/spiffs/messages.bin"

/* Bound update-only journal growth even when the number of logical messages
 * does not increase. The checkpoint is best-effort after a mutation that is
 * already durable; failure leaves the journal authoritative and retryable. */
#define MOG_JOURNAL_CHECKPOINT_OPS 256u

static bool s_initialized = false;
static stored_msg_t *s_durable_records = NULL;
static mog_store_state_t s_state;
static stored_msg_t s_replay_scratch;
static uint64_t s_generation = 0;
static uint64_t s_snapshot_watermark = 0;
static uint64_t s_next_sequence = 1;
static uint32_t s_ops_since_checkpoint = 0;

static uint32_t durable_key(const void *record, void *ctx) {
    (void)ctx;
    return ((const stored_msg_t *)record)->uid;
}

static const stored_msg_t *durable_record_at(uint32_t index) {
    if (!s_durable_records || index >= s_state.count) {
        return NULL;
    }
    return &s_durable_records[index];
}

static int find_uid(uint32_t uid) {
    if (uid == 0) {
        return -1;
    }
    for (uint32_t i = 0; i < s_state.count; ++i) {
        if (s_durable_records[i].uid == uid) {
            return (int)i;
        }
    }
    return -1;
}

static void reset_runtime_metadata(void) {
    s_generation = 0;
    s_snapshot_watermark = 0;
    s_next_sequence = 1;
    s_ops_since_checkpoint = 0;
}

static int allocate_state(void) {
    if (s_durable_records) {
        return 0;
    }

    const size_t bytes = (size_t)CONFIG_BRAMBLE_MSG_PERSIST_MAX * sizeof(stored_msg_t);
    s_durable_records = (stored_msg_t *)heap_caps_calloc(
        CONFIG_BRAMBLE_MSG_PERSIST_MAX, sizeof(stored_msg_t), MALLOC_CAP_SPIRAM);
    if (!s_durable_records) {
        s_durable_records = (stored_msg_t *)heap_caps_calloc(
            CONFIG_BRAMBLE_MSG_PERSIST_MAX, sizeof(stored_msg_t), MALLOC_CAP_DEFAULT);
    }
    if (!s_durable_records) {
        ESP_LOGE(TAG, "durable state allocation failed (%u bytes)", (unsigned)bytes);
        return -1;
    }

    if (mog_store_state_init(&s_state, s_durable_records,
                             CONFIG_BRAMBLE_MSG_PERSIST_MAX,
                             sizeof(stored_msg_t), durable_key, NULL) != MOG_STATE_OK) {
        heap_caps_free(s_durable_records);
        s_durable_records = NULL;
        return -1;
    }
    return 0;
}

static int recover_state(void) {
    mog_store_recovery_info_t info = {0};
    const int rc = mog_store_state_recover(
        &s_state, MOG_SLOT_A_PATH, MOG_SLOT_B_PATH, MOG_JOURNAL_PATH,
        &s_replay_scratch, sizeof(s_replay_scratch), &info);

    if (rc < 0) {
        ESP_LOGE(TAG, "durable state recovery failed: %d", rc);
        return -1;
    }

    if (rc == MOG_STATE_RECOVERED_PARTIAL) {
        ESP_LOGW(TAG, "repairing torn journal tail at %u bytes",
                 (unsigned)info.journal_valid_bytes);
        if (mog_store_journal_truncate(MOG_JOURNAL_PATH,
                                       info.journal_valid_bytes) != MOG_JOURNAL_OK) {
            ESP_LOGE(TAG, "journal tail repair failed");
            return -1;
        }
    }

    s_generation = info.snapshot_generation;
    s_snapshot_watermark = info.snapshot_watermark;
    s_next_sequence = info.next_sequence;
    if (s_next_sequence == 0) {
        ESP_LOGE(TAG, "invalid next journal sequence");
        return -1;
    }

    const uint64_t unapplied_span =
        info.journal_max_sequence > info.snapshot_watermark
            ? info.journal_max_sequence - info.snapshot_watermark
            : 0;
    s_ops_since_checkpoint = unapplied_span > UINT32_MAX
                                 ? UINT32_MAX
                                 : (uint32_t)unapplied_span;

    /* A previous checkpoint may have committed its snapshot and then lost
     * power before truncating a journal that contains only already-checkpointed
     * entries. Once recovery proves that condition, dropping that journal is
     * safe and prevents stale bytes from accumulating forever. */
    if (info.snapshot_present && info.journal_max_sequence != 0 &&
        info.journal_max_sequence <= info.snapshot_watermark) {
        if (mog_store_journal_truncate(MOG_JOURNAL_PATH, 0) == MOG_JOURNAL_OK) {
            s_ops_since_checkpoint = 0;
        }
    }

    ESP_LOGI(TAG,
             "recovered %" PRIu32 " durable messages, generation=%" PRIu64
             ", next_seq=%" PRIu64,
             s_state.count, s_generation, s_next_sequence);
    return 0;
}

static int checkpoint_state(const mog_store_state_t *source, uint64_t last_sequence) {
    uint64_t generation = 0;
    /* API is non-const because checkpoint currently accepts the state object;
     * it does not mutate the record set. */
    if (mog_store_state_checkpoint((mog_store_state_t *)source,
                                   MOG_SLOT_A_PATH, MOG_SLOT_B_PATH,
                                   MOG_JOURNAL_PATH, s_generation,
                                   last_sequence, &generation) != MOG_STATE_OK) {
        return -1;
    }
    s_generation = generation;
    s_snapshot_watermark = last_sequence;
    s_ops_since_checkpoint = 0;
    return 0;
}

static void maybe_checkpoint_updates(void) {
    if (s_ops_since_checkpoint < MOG_JOURNAL_CHECKPOINT_OPS) {
        return;
    }
    const uint64_t last_sequence = s_next_sequence - 1;
    if (checkpoint_state(&s_state, last_sequence) != 0) {
        ESP_LOGW(TAG, "periodic journal checkpoint failed; journal remains authoritative");
    }
}

static int append_put(const stored_msg_t *msg) {
    if (!msg || msg->uid == 0 || s_next_sequence == UINT64_MAX) {
        return -1;
    }

    const uint64_t sequence = s_next_sequence;
    if (mog_store_journal_append(MOG_JOURNAL_PATH, MOG_STORE_JOURNAL_PUT,
                                 sequence, msg->uid, msg, sizeof(*msg),
                                 sizeof(*msg)) != MOG_JOURNAL_OK) {
        return -1;
    }

    const int state_rc = mog_store_state_put(&s_state, msg->uid, msg, sizeof(*msg));
    if (state_rc != MOG_STATE_OK) {
        /* Journal is already durable. Rebuild the mirror from disk rather than
         * pretending RAM still represents committed state. */
        ESP_LOGE(TAG, "durable mirror update failed after journal commit: %d", state_rc);
        if (recover_state() != 0) {
            s_initialized = false;
        }
        return -1;
    }

    s_next_sequence++;
    if (s_ops_since_checkpoint != UINT32_MAX) {
        s_ops_since_checkpoint++;
    }
    maybe_checkpoint_updates();
    return 0;
}

int msg_store_spiffs_init(void) {
    if (!esp_spiffs_mounted(NULL)) {
        ESP_LOGW(TAG, "SPIFFS not mounted, persistence disabled");
        return -1;
    }
    if (allocate_state() != 0) {
        return -1;
    }

    mog_store_state_clear(&s_state);
    reset_runtime_metadata();
    if (recover_state() != 0) {
        /* Fail closed: message persistence can degrade without touching the
         * independent NVS identity/config store. Unknown bytes are not erased
         * or reinterpreted automatically. */
        s_initialized = false;
        return -1;
    }

    s_initialized = true;
    if (access(LEGACY_MSG_PATH, F_OK) == 0 && s_state.count == 0) {
        ESP_LOGW(TAG,
                 "legacy Bramble message file present; left untouched (no implicit migration)");
    }
    return 0;
}

int msg_store_spiffs_save(const stored_msg_t *msg) {
    if (!s_initialized || !msg || msg->uid == 0) {
        return -1;
    }
    if (find_uid(msg->uid) >= 0) {
        ESP_LOGE(TAG, "save rejected duplicate uid=%" PRIu32, msg->uid);
        return -1;
    }
    if (s_state.count >= s_state.capacity) {
        ESP_LOGE(TAG, "durable store full before rollover");
        return -1;
    }
    return append_put(msg);
}

int msg_store_spiffs_update(int from_end, const stored_msg_t *msg) {
    if (!s_initialized || !msg || msg->uid == 0 || from_end < 0) {
        return -1;
    }

    const int index = find_uid(msg->uid);
    if (index < 0) {
        ESP_LOGW(TAG, "update uid=%" PRIu32 " not found", msg->uid);
        return -1;
    }

    const stored_msg_t *current = durable_record_at((uint32_t)index);
    if (!current || !msg_store_record_matches(current, msg)) {
        ESP_LOGW(TAG, "update uid=%" PRIu32 " immutable fields drifted", msg->uid);
        return -1;
    }

    /* from_end remains part of the stable Bramble API, but UID is now the
     * durable lookup key. This removes the old positional/in-place rewrite
     * hazard while preserving caller compatibility. */
    (void)from_end;
    return append_put(msg);
}

int msg_store_spiffs_get_count(void) {
    return s_initialized ? (int)s_state.count : 0;
}

int msg_store_spiffs_load_recent(stored_msg_t *msgs, int max_count) {
    if (!s_initialized || !msgs || max_count <= 0) {
        return 0;
    }
    return mog_store_state_copy_recent(&s_state, msgs, (uint32_t)max_count);
}

void msg_store_spiffs_rollover(int max_messages, int keep_pct) {
    if (!s_initialized || max_messages <= 0 || s_state.count < (uint32_t)max_messages) {
        return;
    }
    if (keep_pct < 50 || keep_pct > 90) {
        ESP_LOGW(TAG, "invalid keep_pct=%d; using 75", keep_pct);
        keep_pct = 75;
    }

    uint32_t keep_count = (uint32_t)(((uint64_t)(uint32_t)max_messages *
                                      (uint32_t)keep_pct) / 100u);
    if (keep_count > s_state.count) {
        keep_count = s_state.count;
    }
    if (keep_count == 0) {
        ESP_LOGE(TAG, "rollover produced zero keep_count");
        return;
    }

    stored_msg_t *compact_records = (stored_msg_t *)heap_caps_calloc(
        keep_count, sizeof(stored_msg_t), MALLOC_CAP_SPIRAM);
    if (!compact_records) {
        compact_records = (stored_msg_t *)heap_caps_calloc(
            keep_count, sizeof(stored_msg_t), MALLOC_CAP_DEFAULT);
    }
    if (!compact_records) {
        ESP_LOGE(TAG, "rollover scratch allocation failed");
        return;
    }

    mog_store_state_t compact;
    if (mog_store_state_init(&compact, compact_records, keep_count,
                             sizeof(stored_msg_t), durable_key, NULL) != MOG_STATE_OK) {
        heap_caps_free(compact_records);
        return;
    }

    const uint32_t start = s_state.count - keep_count;
    int build_ok = 1;
    for (uint32_t i = start; i < s_state.count; ++i) {
        const stored_msg_t *record = durable_record_at(i);
        if (!record || mog_store_state_put(&compact, record->uid, record,
                                           sizeof(*record)) != MOG_STATE_OK) {
            build_ok = 0;
            break;
        }
    }

    const uint64_t last_sequence = s_next_sequence - 1;
    if (!build_ok || checkpoint_state(&compact, last_sequence) != 0) {
        ESP_LOGE(TAG, "transactional rollover failed; previous durable state retained");
        heap_caps_free(compact_records);
        return;
    }

    memcpy(s_durable_records, compact_records,
           (size_t)keep_count * sizeof(stored_msg_t));
    s_state.count = keep_count;
    heap_caps_free(compact_records);

    ESP_LOGI(TAG, "transactional rollover retained %" PRIu32 " messages", keep_count);
}

void msg_store_spiffs_clear(void) {
    unlink(MOG_SLOT_A_PATH);
    unlink(MOG_SLOT_B_PATH);
    unlink(MOG_JOURNAL_PATH);
    /* Clear means clear all message persistence, including an untouched legacy
     * Bramble store if one exists. Identity/config live elsewhere and are not
     * affected. */
    unlink(LEGACY_MSG_PATH);

    if (s_durable_records) {
        memset(s_durable_records, 0,
               (size_t)s_state.capacity * sizeof(stored_msg_t));
        mog_store_state_clear(&s_state);
    }
    reset_runtime_metadata();
    s_initialized = esp_spiffs_mounted(NULL) && s_durable_records != NULL;
    ESP_LOGI(TAG, "cleared durable message store");
}

#else /* Host stubs retain the upstream public contract. */

int msg_store_spiffs_init(void) { return -1; }
int msg_store_spiffs_save(const stored_msg_t *msg) {
    (void)msg;
    return -1;
}
int msg_store_spiffs_update(int from_end, const stored_msg_t *msg) {
    (void)from_end;
    (void)msg;
    return -1;
}
int msg_store_spiffs_get_count(void) { return 0; }
int msg_store_spiffs_load_recent(stored_msg_t *msgs, int max_count) {
    (void)msgs;
    (void)max_count;
    return 0;
}
void msg_store_spiffs_rollover(int max_messages, int keep_pct) {
    (void)max_messages;
    (void)keep_pct;
}
void msg_store_spiffs_clear(void) {}

#endif /* ESP_PLATFORM */
