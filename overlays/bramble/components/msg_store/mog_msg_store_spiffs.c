/*
 * Firmware-mash durable message-store adapter for the pinned Bramble T-Deck
 * foundation. It implements Bramble's existing msg_store_spiffs.h API so the
 * upstream in-memory ring, UI and reliability callers remain the single
 * application-level message-store implementation.
 *
 * Durable model:
 *   - normal saves/updates are append-only committed journal PUT operations;
 *   - compaction writes a complete snapshot to an inactive generation;
 *   - boot selects the newest valid snapshot, then replays newer journal ops;
 *   - a torn/corrupt journal tail is truncated before any new append;
 *   - identity/config storage is never touched by this backend.
 */
#include "msg_store_spiffs.h"
#include "msg_store.h"
#include "mog_store_journal.h"
#include "mog_store_snapshot.h"

#include <string.h>

#ifdef ESP_PLATFORM

#include "sdkconfig.h"
#include "esp_heap_caps.h"
#include "esp_log.h"
#include "esp_spiffs.h"

#include <errno.h>
#include <stdio.h>
#include <unistd.h>

#define TAG "mog_msg_store"

#define MOG_SLOT_A_PATH "/spiffs/mog_messages_a.bin"
#define MOG_SLOT_B_PATH "/spiffs/mog_messages_b.bin"
#define MOG_JOURNAL_PATH "/spiffs/mog_messages.jrn"
#define MOG_LEGACY_PATH "/spiffs/messages.bin"
#define MOG_LEGACY_BACKUP_PATH "/spiffs/messages.legacy"

#define MOG_LEGACY_MAGIC 0x4252414Du /* BRAM */
#define MOG_LEGACY_VERSION 1u

#ifndef CONFIG_BRAMBLE_MSG_PERSIST_MAX
#define CONFIG_BRAMBLE_MSG_PERSIST_MAX 300
#endif
#ifndef CONFIG_BRAMBLE_MSG_PERSIST_ROLLOVER_KEEP_PCT
#define CONFIG_BRAMBLE_MSG_PERSIST_ROLLOVER_KEEP_PCT 75
#endif

#define MOG_STATE_CAPACITY ((uint32_t)CONFIG_BRAMBLE_MSG_PERSIST_MAX + 1u)
#define MOG_JOURNAL_COMPACT_FACTOR 4u
#define MOG_MIN_JOURNAL_COMPACT_ENTRIES 256u

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t record_size;
    uint32_t record_count;
    uint32_t next_id;
} legacy_header_t;

static stored_msg_t *s_records;
static uint32_t s_count;
static uint32_t s_capacity;
static uint64_t s_generation;
static uint64_t s_last_sequence;
static uint64_t s_snapshot_watermark;
static uint64_t s_journal_entries;
static bool s_initialized;
static bool s_writable;
static char s_active_slot[40];
static stored_msg_t s_replay_scratch;

static bool file_exists(const char *path) {
    return path && access(path, F_OK) == 0;
}

static void reset_runtime_state(void) {
    s_count = 0;
    s_generation = 0;
    s_last_sequence = 0;
    s_snapshot_watermark = 0;
    s_journal_entries = 0;
    s_initialized = false;
    s_writable = false;
    s_active_slot[0] = '\0';
    if (s_records && s_capacity > 0) {
        memset(s_records, 0, (size_t)s_capacity * sizeof(*s_records));
    }
}

static bool ensure_state_buffer(void) {
    if (s_records) {
        return true;
    }

    s_capacity = MOG_STATE_CAPACITY;
    if (s_capacity == 0 || s_capacity > 4096u) {
        ESP_LOGE(TAG, "invalid persistence capacity: %lu", (unsigned long)s_capacity);
        return false;
    }

    const size_t bytes = (size_t)s_capacity * sizeof(*s_records);
    s_records = heap_caps_calloc(s_capacity, sizeof(*s_records), MALLOC_CAP_SPIRAM);
    if (!s_records) {
        s_records = heap_caps_calloc(s_capacity, sizeof(*s_records), MALLOC_CAP_DEFAULT);
    }
    if (!s_records) {
        ESP_LOGE(TAG, "cannot allocate %u bytes for durable cache", (unsigned)bytes);
        return false;
    }
    return true;
}

static bool record_semantically_valid(const stored_msg_t *msg) {
    if (!msg || msg->uid == 0 || msg->text_len >= MSG_TEXT_MAX ||
        msg->route_hop_count > MSG_ROUTE_MAX_HOPS) {
        return false;
    }
    if (msg->direction < MSG_DIR_INCOMING || msg->direction > MSG_DIR_BROADCAST_OUT) {
        return false;
    }
    if (msg->status < MSG_STATUS_NONE || msg->status > MSG_STATUS_QUEUED) {
        return false;
    }
    if (msg->channel_index < MSG_STORE_DM_CHANNEL || msg->channel_index > 255) {
        return false;
    }
    return true;
}

static int find_uid(uint32_t uid) {
    if (uid == 0) {
        return -1;
    }
    for (uint32_t i = 0; i < s_count; ++i) {
        if (s_records[i].uid == uid) {
            return (int)i;
        }
    }
    return -1;
}

static bool all_records_valid_and_unique(void) {
    for (uint32_t i = 0; i < s_count; ++i) {
        if (!record_semantically_valid(&s_records[i])) {
            return false;
        }
        for (uint32_t j = i + 1u; j < s_count; ++j) {
            if (s_records[i].uid == s_records[j].uid) {
                return false;
            }
        }
    }
    return true;
}

static int apply_put(uint32_t uid, const stored_msg_t *msg) {
    if (!record_semantically_valid(msg) || uid == 0 || msg->uid != uid) {
        return -1;
    }
    const int existing = find_uid(uid);
    if (existing >= 0) {
        if (!msg_store_record_matches(&s_records[existing], msg)) {
            return -1;
        }
        s_records[existing] = *msg;
        return 0;
    }
    if (s_count >= s_capacity) {
        return -1;
    }
    s_records[s_count++] = *msg;
    return 0;
}

static int apply_delete(uint32_t uid) {
    const int index = find_uid(uid);
    if (index < 0) {
        return 0; /* idempotent obsolete delete */
    }
    const uint32_t i = (uint32_t)index;
    if (i + 1u < s_count) {
        memmove(&s_records[i], &s_records[i + 1u],
                (size_t)(s_count - i - 1u) * sizeof(*s_records));
    }
    --s_count;
    memset(&s_records[s_count], 0, sizeof(*s_records));
    return 0;
}

static int replay_cb(mog_store_journal_op_t op,
                     uint64_t sequence,
                     uint32_t uid,
                     const void *payload,
                     uint32_t payload_size,
                     void *ctx) {
    (void)ctx;
    if (sequence <= s_snapshot_watermark) {
        return -1; /* journal API must filter these before callback */
    }
    if (op == MOG_STORE_JOURNAL_PUT) {
        if (!payload || payload_size != sizeof(stored_msg_t)) {
            return -1;
        }
        return apply_put(uid, (const stored_msg_t *)payload);
    }
    if (op == MOG_STORE_JOURNAL_DELETE) {
        return payload == NULL && payload_size == 0 ? apply_delete(uid) : -1;
    }
    return -1;
}

static const char *inactive_slot(void) {
    return strcmp(s_active_slot, MOG_SLOT_A_PATH) == 0 ? MOG_SLOT_B_PATH : MOG_SLOT_A_PATH;
}

static int compact_keep_newest(uint32_t keep_count) {
    if (!s_initialized || !s_writable || !s_records) {
        return -1;
    }
    if (keep_count > s_count) {
        keep_count = s_count;
    }

    const uint32_t skip = s_count - keep_count;
    const stored_msg_t *snapshot_records = keep_count > 0 ? &s_records[skip] : NULL;
    const uint64_t new_generation = s_generation + 1u;
    if (new_generation == 0) {
        ESP_LOGE(TAG, "snapshot generation exhausted");
        return -1;
    }

    const char *target = inactive_slot();
    const int write_rc = mog_store_snapshot_write(target, new_generation, s_last_sequence,
                                                   snapshot_records, keep_count,
                                                   sizeof(stored_msg_t));
    if (write_rc != MOG_STORE_OK) {
        ESP_LOGE(TAG, "snapshot write failed: %d", write_rc);
        return -1;
    }

    mog_store_snapshot_info_t verify = {0};
    const int verify_rc = mog_store_snapshot_validate(target, sizeof(stored_msg_t), &verify);
    if (verify_rc != MOG_STORE_OK || verify.generation != new_generation ||
        verify.last_sequence != s_last_sequence || verify.record_count != keep_count) {
        ESP_LOGE(TAG, "snapshot verification failed: %d", verify_rc);
        return -1;
    }

    if (skip > 0 && keep_count > 0) {
        memmove(s_records, &s_records[skip], (size_t)keep_count * sizeof(*s_records));
    }
    if (keep_count < s_count) {
        memset(&s_records[keep_count], 0,
               (size_t)(s_count - keep_count) * sizeof(*s_records));
    }
    s_count = keep_count;
    s_generation = new_generation;
    s_snapshot_watermark = s_last_sequence;
    snprintf(s_active_slot, sizeof(s_active_slot), "%s", target);

    if (mog_store_journal_truncate(MOG_JOURNAL_PATH, 0) == MOG_JOURNAL_OK) {
        s_journal_entries = 0;
    } else {
        /*
         * Snapshot is already durable, so committed data is safe. But the
         * journal file state is now unknown. Never append behind it this boot.
         */
        ESP_LOGE(TAG, "journal cleanup failed after snapshot; switching store read-only");
        s_writable = false;
        return -1;
    }

    return 0;
}

static uint64_t journal_compact_threshold(void) {
    uint64_t threshold = (uint64_t)CONFIG_BRAMBLE_MSG_PERSIST_MAX *
                         (uint64_t)MOG_JOURNAL_COMPACT_FACTOR;
    if (threshold < MOG_MIN_JOURNAL_COMPACT_ENTRIES) {
        threshold = MOG_MIN_JOURNAL_COMPACT_ENTRIES;
    }
    return threshold;
}

static void maybe_compact_journal(void) {
    if (s_writable && s_journal_entries >= journal_compact_threshold()) {
        if (compact_keep_newest(s_count) != 0 && s_writable) {
            ESP_LOGW(TAG, "background journal compaction failed; committed journal kept");
        }
    }
}

static int append_put(const stored_msg_t *msg) {
    if (!record_semantically_valid(msg) || !s_writable) {
        return -1;
    }
    const uint64_t sequence = s_last_sequence + 1u;
    if (sequence == 0) {
        return -1;
    }
    const int rc = mog_store_journal_append(MOG_JOURNAL_PATH, MOG_STORE_JOURNAL_PUT,
                                            sequence, msg->uid, msg, sizeof(*msg),
                                            sizeof(*msg));
    if (rc != MOG_JOURNAL_OK) {
        ESP_LOGE(TAG, "journal PUT failed for uid=%lu: %d",
                 (unsigned long)msg->uid, rc);
        return -1;
    }
    s_last_sequence = sequence;
    ++s_journal_entries;
    return 0;
}

static int load_snapshot_if_present(void) {
    const bool a_exists = file_exists(MOG_SLOT_A_PATH);
    const bool b_exists = file_exists(MOG_SLOT_B_PATH);
    if (!a_exists && !b_exists) {
        return 0;
    }

    char selected[sizeof(s_active_slot)] = {0};
    mog_store_snapshot_info_t info = {0};
    const int select_rc = mog_store_snapshot_select(MOG_SLOT_A_PATH, MOG_SLOT_B_PATH,
                                                     sizeof(stored_msg_t), selected,
                                                     sizeof(selected), &info);
    if (select_rc != MOG_STORE_OK) {
        ESP_LOGE(TAG, "snapshot files exist but none is valid: %d", select_rc);
        return -1;
    }
    if (info.record_count > s_capacity) {
        ESP_LOGE(TAG, "snapshot has %lu records, capacity is %lu",
                 (unsigned long)info.record_count, (unsigned long)s_capacity);
        return -1;
    }

    const int read_rc = mog_store_snapshot_read(selected, sizeof(stored_msg_t), s_records,
                                                s_capacity, &info);
    if (read_rc != MOG_STORE_OK) {
        ESP_LOGE(TAG, "snapshot read failed: %d", read_rc);
        return -1;
    }

    s_count = info.record_count;
    if (!all_records_valid_and_unique()) {
        ESP_LOGE(TAG, "snapshot integrity CRC passed but record semantics are invalid");
        return -1;
    }
    s_generation = info.generation;
    s_snapshot_watermark = info.last_sequence;
    s_last_sequence = info.last_sequence;
    snprintf(s_active_slot, sizeof(s_active_slot), "%s", selected);
    return 1;
}

static int migrate_legacy_if_present(void) {
    if (!file_exists(MOG_LEGACY_PATH)) {
        return 0;
    }

    FILE *f = fopen(MOG_LEGACY_PATH, "rb");
    if (!f) {
        return -1;
    }
    legacy_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1 ||
        header.magic != MOG_LEGACY_MAGIC || header.version != MOG_LEGACY_VERSION ||
        header.record_size != sizeof(stored_msg_t)) {
        fclose(f);
        ESP_LOGE(TAG, "legacy message store header is not safely migratable");
        return -1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    const long file_size = ftell(f);
    if (file_size < (long)sizeof(header)) {
        fclose(f);
        return -1;
    }
    const long payload = file_size - (long)sizeof(header);
    const uint32_t actual = (uint32_t)(payload / (long)sizeof(stored_msg_t));
    const uint32_t keep = actual < s_capacity ? actual : s_capacity;
    const uint32_t skip = actual - keep;
    const long offset = (long)sizeof(header) + (long)skip * (long)sizeof(stored_msg_t);
    if (fseek(f, offset, SEEK_SET) != 0 ||
        (keep > 0 && fread(s_records, sizeof(stored_msg_t), keep, f) != keep)) {
        fclose(f);
        return -1;
    }
    if (fclose(f) != 0) {
        return -1;
    }

    s_count = keep;
    if (!all_records_valid_and_unique()) {
        ESP_LOGE(TAG, "legacy records failed semantic/UID validation; leaving legacy file intact");
        memset(s_records, 0, (size_t)s_capacity * sizeof(*s_records));
        s_count = 0;
        return -1;
    }

    const int write_rc = mog_store_snapshot_write(MOG_SLOT_A_PATH, 1, 0,
                                                   keep > 0 ? s_records : NULL,
                                                   keep, sizeof(stored_msg_t));
    mog_store_snapshot_info_t verify = {0};
    if (write_rc != MOG_STORE_OK ||
        mog_store_snapshot_validate(MOG_SLOT_A_PATH, sizeof(stored_msg_t), &verify) !=
            MOG_STORE_OK ||
        verify.record_count != keep || verify.generation != 1 || verify.last_sequence != 0) {
        ESP_LOGE(TAG, "legacy migration could not commit a valid snapshot");
        memset(s_records, 0, (size_t)s_capacity * sizeof(*s_records));
        s_count = 0;
        return -1;
    }

    s_generation = 1;
    s_snapshot_watermark = 0;
    s_last_sequence = 0;
    snprintf(s_active_slot, sizeof(s_active_slot), "%s", MOG_SLOT_A_PATH);

    /* Keep one diagnostic fallback instead of destroying the legacy bytes. */
    unlink(MOG_LEGACY_BACKUP_PATH);
    if (rename(MOG_LEGACY_PATH, MOG_LEGACY_BACKUP_PATH) != 0) {
        ESP_LOGW(TAG, "new snapshot committed but legacy backup rename failed: errno=%d", errno);
    }
    return 1;
}

int msg_store_spiffs_init(void) {
    if (!esp_spiffs_mounted(NULL)) {
        ESP_LOGW(TAG, "SPIFFS not mounted, persistence disabled");
        return -1;
    }
    if (!ensure_state_buffer()) {
        return -1;
    }
    reset_runtime_state();

    const int snapshot_state = load_snapshot_if_present();
    if (snapshot_state < 0) {
        return -1; /* fail closed; never overwrite unknown/corrupt generations */
    }
    if (snapshot_state == 0) {
        const int migration = migrate_legacy_if_present();
        if (migration < 0) {
            return -1;
        }
    }

    mog_store_journal_scan_info_t scan = {0};
    const int replay_rc = mog_store_journal_replay(MOG_JOURNAL_PATH, sizeof(stored_msg_t),
                                                    s_snapshot_watermark,
                                                    &s_replay_scratch,
                                                    sizeof(s_replay_scratch),
                                                    replay_cb, NULL, &scan);
    if (replay_rc != MOG_JOURNAL_OK && replay_rc != MOG_JOURNAL_RECOVERED_PARTIAL) {
        ESP_LOGE(TAG, "journal replay failed: %d", replay_rc);
        return -1;
    }

    if (scan.max_sequence > s_last_sequence) {
        s_last_sequence = scan.max_sequence;
    }
    s_journal_entries = scan.valid_entries;

    s_writable = true;
    if (replay_rc == MOG_JOURNAL_RECOVERED_PARTIAL) {
        if (mog_store_journal_truncate(MOG_JOURNAL_PATH, scan.valid_bytes) != MOG_JOURNAL_OK) {
            ESP_LOGE(TAG, "journal tail repair failed; persistence is read-only this boot");
            s_writable = false;
        } else {
            ESP_LOGW(TAG, "recovered and truncated a damaged journal tail");
        }
    }

    s_initialized = true;
    ESP_LOGI(TAG, "durable store ready: %lu records, gen=%llu, seq=%llu%s",
             (unsigned long)s_count,
             (unsigned long long)s_generation,
             (unsigned long long)s_last_sequence,
             s_writable ? "" : " (read-only)");
    return 0;
}

int msg_store_spiffs_save(const stored_msg_t *msg) {
    if (!s_initialized || !s_writable || !record_semantically_valid(msg)) {
        return -1;
    }
    if (find_uid(msg->uid) >= 0) {
        ESP_LOGE(TAG, "save refused duplicate uid=%lu", (unsigned long)msg->uid);
        return -1;
    }

    if (s_count >= s_capacity) {
        const uint32_t keep = ((uint32_t)CONFIG_BRAMBLE_MSG_PERSIST_MAX *
                               (uint32_t)CONFIG_BRAMBLE_MSG_PERSIST_ROLLOVER_KEEP_PCT) /
                              100u;
        if (compact_keep_newest(keep) != 0 || !s_writable || s_count >= s_capacity) {
            ESP_LOGE(TAG, "store full and compaction failed");
            return -1;
        }
    }

    if (append_put(msg) != 0) {
        return -1;
    }
    if (apply_put(msg->uid, msg) != 0) {
        /* Journal commit succeeded: do not permit new writes over diverged RAM state. */
        ESP_LOGE(TAG, "RAM durability cache diverged after committed PUT; switching read-only");
        s_writable = false;
        return -1;
    }
    maybe_compact_journal();
    return 0;
}

int msg_store_spiffs_update(int from_end, const stored_msg_t *msg) {
    if (!s_initialized || !s_writable || !msg || from_end < 0 ||
        (uint32_t)from_end >= s_count) {
        return -1;
    }

    const uint32_t index = s_count - 1u - (uint32_t)from_end;
    if (!msg_store_record_matches(&s_records[index], msg)) {
        ESP_LOGW(TAG, "update mapping drift for from_end=%d; refusing write", from_end);
        return -1;
    }

    stored_msg_t updated = *msg;
    updated.timestamp_s = s_records[index].timestamp_s;
    if (!record_semantically_valid(&updated) || append_put(&updated) != 0) {
        return -1;
    }
    s_records[index] = updated;
    maybe_compact_journal();
    return 0;
}

int msg_store_spiffs_get_count(void) {
    return s_initialized ? (int)s_count : 0;
}

int msg_store_spiffs_load_recent(stored_msg_t *msgs, int max_count) {
    if (!s_initialized || !msgs || max_count <= 0) {
        return 0;
    }
    uint32_t to_load = s_count;
    if (to_load > (uint32_t)max_count) {
        to_load = (uint32_t)max_count;
    }
    const uint32_t start = s_count - to_load;
    if (to_load > 0) {
        memcpy(msgs, &s_records[start], (size_t)to_load * sizeof(*msgs));
    }
    return (int)to_load;
}

void msg_store_spiffs_rollover(int max_messages, int keep_pct) {
    if (!s_initialized || !s_writable || max_messages <= 0) {
        return;
    }
    if (keep_pct < 50 || keep_pct > 90) {
        keep_pct = 75;
    }

    if (s_count > (uint32_t)max_messages) {
        const uint32_t keep = ((uint32_t)max_messages * (uint32_t)keep_pct) / 100u;
        if (compact_keep_newest(keep) != 0 && s_writable) {
            ESP_LOGW(TAG, "retention compaction failed; committed state retained");
        }
    } else {
        maybe_compact_journal();
    }
}

void msg_store_spiffs_clear(void) {
    const char *paths[] = {
        MOG_SLOT_A_PATH,
        MOG_SLOT_B_PATH,
        MOG_JOURNAL_PATH,
        MOG_LEGACY_PATH,
        MOG_LEGACY_BACKUP_PATH,
    };
    for (size_t i = 0; i < sizeof(paths) / sizeof(paths[0]); ++i) {
        if (file_exists(paths[i]) && unlink(paths[i]) != 0) {
            ESP_LOGE(TAG, "clear could not remove %s (errno=%d)", paths[i], errno);
        }
    }
    reset_runtime_state();
    ESP_LOGI(TAG, "cleared durable message persistence; re-init required");
}

#else /* ESP_PLATFORM */

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
