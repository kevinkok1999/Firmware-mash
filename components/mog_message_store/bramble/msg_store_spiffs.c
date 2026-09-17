#include "msg_store_spiffs.h"
#include "mog_store_journal.h"
#include "mog_store_snapshot.h"
#include "mog_store_state.h"

#ifdef ESP_PLATFORM
#include "sdkconfig.h"
#include "esp_log.h"
#include "esp_spiffs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define TAG "mog_msg_store"
#define SNAPSHOT_A "/spiffs/mog-msg-a.bin"
#define SNAPSHOT_B "/spiffs/mog-msg-b.bin"
#define JOURNAL_PATH "/spiffs/mog-msg.journal"
#define LEGACY_PATH "/spiffs/messages.bin"
#define LEGACY_MAGIC 0x4252414du
#define LEGACY_VERSION 1u
#define LEGACY_HEADER_SIZE 16u

static bool s_initialized = false;
static stored_msg_t *s_records = NULL;
static stored_msg_t s_scratch;
static mog_store_state_t s_state;
static uint64_t s_next_sequence = 1;
static uint64_t s_generation = 0;
static int s_active_count = 0;

static uint16_t get_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static uint32_t get_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint32_t record_key(const void *record, void *ctx) {
    (void)ctx;
    return ((const stored_msg_t *)record)->uid;
}

static int ensure_state_buffer(void) {
    if (!s_records) {
        s_records = calloc(CONFIG_BRAMBLE_MSG_PERSIST_MAX, sizeof(stored_msg_t));
        if (!s_records) {
            ESP_LOGE(TAG, "state buffer allocation failed");
            return -1;
        }
    }
    if (mog_store_state_init(&s_state, s_records, CONFIG_BRAMBLE_MSG_PERSIST_MAX,
                             sizeof(stored_msg_t), record_key, NULL) != MOG_STATE_OK) {
        return -1;
    }
    return 0;
}

static bool any_new_store_file_exists(void) {
    return access(SNAPSHOT_A, F_OK) == 0 || access(SNAPSHOT_B, F_OK) == 0 ||
           access(JOURNAL_PATH, F_OK) == 0;
}

static int validate_loaded_legacy(uint32_t count) {
    for (uint32_t i = 0; i < count; ++i) {
        if (s_records[i].uid == 0) {
            return -1;
        }
        for (uint32_t j = 0; j < i; ++j) {
            if (s_records[j].uid == s_records[i].uid) {
                return -1;
            }
        }
    }
    return 0;
}

static int migrate_legacy_if_needed(void) {
    if (any_new_store_file_exists()) {
        return 0;
    }

    FILE *f = fopen(LEGACY_PATH, "rb");
    if (!f) {
        return 0;
    }

    uint8_t header[LEGACY_HEADER_SIZE];
    if (fread(header, 1, sizeof(header), f) != sizeof(header) ||
        get_u32_le(header) != LEGACY_MAGIC ||
        get_u16_le(header + 4) != LEGACY_VERSION ||
        get_u16_le(header + 6) != sizeof(stored_msg_t)) {
        fclose(f);
        ESP_LOGE(TAG, "legacy store format is not safely migratable");
        return -1;
    }

    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return -1;
    }
    const long file_size = ftell(f);
    if (file_size < (long)LEGACY_HEADER_SIZE) {
        fclose(f);
        return -1;
    }

    const long payload = file_size - (long)LEGACY_HEADER_SIZE;
    const uint32_t actual_count = (uint32_t)(payload / (long)sizeof(stored_msg_t));
    uint32_t load_count = actual_count;
    if (load_count > CONFIG_BRAMBLE_MSG_PERSIST_MAX) {
        load_count = CONFIG_BRAMBLE_MSG_PERSIST_MAX;
    }
    const uint32_t skip = actual_count - load_count;
    const long offset = (long)LEGACY_HEADER_SIZE + (long)skip * (long)sizeof(stored_msg_t);
    if (fseek(f, offset, SEEK_SET) != 0 ||
        (load_count > 0 && fread(s_records, sizeof(stored_msg_t), load_count, f) != load_count)) {
        fclose(f);
        return -1;
    }
    fclose(f);

    if (validate_loaded_legacy(load_count) != 0) {
        ESP_LOGE(TAG, "legacy store contains invalid/duplicate UIDs; leaving it untouched");
        return -1;
    }

    if (mog_store_snapshot_write(SNAPSHOT_A, 1, 0, s_records, load_count,
                                 sizeof(stored_msg_t)) != MOG_STORE_OK) {
        return -1;
    }
    mog_store_snapshot_info_t verify = {0};
    if (mog_store_snapshot_validate(SNAPSHOT_A, sizeof(stored_msg_t), &verify) != MOG_STORE_OK ||
        verify.generation != 1 || verify.record_count != load_count) {
        unlink(SNAPSHOT_A);
        return -1;
    }

    ESP_LOGI(TAG, "migrated %u legacy messages to transactional store", (unsigned)load_count);
    return 0;
}

static int recover_state(void) {
    mog_store_recovery_info_t info = {0};
    int rc = mog_store_state_recover(&s_state, SNAPSHOT_A, SNAPSHOT_B, JOURNAL_PATH,
                                     &s_scratch, sizeof(s_scratch), &info);
    if (rc == MOG_STATE_RECOVERED_PARTIAL) {
        ESP_LOGW(TAG, "journal tail damaged; truncating to %u valid bytes",
                 (unsigned)info.journal_valid_bytes);
        if (mog_store_journal_truncate(JOURNAL_PATH, info.journal_valid_bytes) !=
            MOG_JOURNAL_OK) {
            return -1;
        }
        rc = MOG_STATE_OK;
    }
    if (rc != MOG_STATE_OK) {
        ESP_LOGE(TAG, "transactional message-store recovery failed: %d", rc);
        return -1;
    }

    s_generation = info.snapshot_generation;
    s_next_sequence = info.next_sequence;
    s_active_count = (int)s_state.count;
    return 0;
}

int msg_store_spiffs_init(void) {
    if (!esp_spiffs_mounted(NULL)) {
        ESP_LOGW(TAG, "SPIFFS not mounted, persistence disabled");
        return -1;
    }
    if (ensure_state_buffer() != 0) {
        return -1;
    }
    if (migrate_legacy_if_needed() != 0) {
        return -1;
    }
    if (recover_state() != 0) {
        if (access(LEGACY_PATH, F_OK) == 0 && access(JOURNAL_PATH, F_OK) != 0) {
            ESP_LOGW(TAG, "retrying interrupted legacy migration");
            unlink(SNAPSHOT_A);
            unlink(SNAPSHOT_B);
            if (migrate_legacy_if_needed() != 0 || recover_state() != 0) {
                return -1;
            }
        } else {
            return -1;
        }
    }
    s_initialized = true;
    ESP_LOGI(TAG, "transactional store ready: %d messages, generation=%llu, next_seq=%llu",
             s_active_count, (unsigned long long)s_generation,
             (unsigned long long)s_next_sequence);
    return 0;
}

void msg_store_spiffs_rollover(int max_messages, int keep_pct);

int msg_store_spiffs_save(const stored_msg_t *msg) {
    if (!s_initialized || !msg || msg->uid == 0 || s_next_sequence == UINT64_MAX) {
        return -1;
    }
    if (s_active_count >= CONFIG_BRAMBLE_MSG_PERSIST_MAX) {
        msg_store_spiffs_rollover(CONFIG_BRAMBLE_MSG_PERSIST_MAX,
                                  CONFIG_BRAMBLE_MSG_PERSIST_ROLLOVER_KEEP_PCT);
        if (s_active_count >= CONFIG_BRAMBLE_MSG_PERSIST_MAX) {
            ESP_LOGE(TAG, "store remains full after rollover; refusing unbounded append");
            return -1;
        }
    }
    if (mog_store_journal_append(JOURNAL_PATH, MOG_STORE_JOURNAL_PUT, s_next_sequence,
                                 msg->uid, msg, sizeof(*msg), sizeof(*msg)) != MOG_JOURNAL_OK) {
        return -1;
    }
    s_next_sequence++;
    const int apply_rc = mog_store_state_put(&s_state, msg->uid, msg, sizeof(*msg));
    if (apply_rc != MOG_STATE_OK) {
        ESP_LOGE(TAG, "durable append committed but RAM state apply failed: %d", apply_rc);
        s_initialized = false;
        return 0;
    }
    s_active_count = (int)s_state.count;
    return 0;
}

int msg_store_spiffs_update(int from_end, const stored_msg_t *msg) {
    (void)from_end;
    if (!s_initialized || !msg || msg->uid == 0 || s_next_sequence == UINT64_MAX) {
        return -1;
    }
    if (mog_store_journal_append(JOURNAL_PATH, MOG_STORE_JOURNAL_PUT, s_next_sequence,
                                 msg->uid, msg, sizeof(*msg), sizeof(*msg)) != MOG_JOURNAL_OK) {
        return -1;
    }
    s_next_sequence++;
    const int apply_rc = mog_store_state_put(&s_state, msg->uid, msg, sizeof(*msg));
    if (apply_rc != MOG_STATE_OK) {
        ESP_LOGE(TAG, "durable update committed but RAM state apply failed: %d", apply_rc);
        s_initialized = false;
        return 0;
    }
    s_active_count = (int)s_state.count;
    return 0;
}

int msg_store_spiffs_get_count(void) {
    return s_initialized ? s_active_count : 0;
}

int msg_store_spiffs_load_recent(stored_msg_t *msgs, int max_count) {
    if (!s_initialized || !msgs || max_count <= 0) {
        return 0;
    }
    return mog_store_state_copy_recent(&s_state, msgs, (uint32_t)max_count);
}

void msg_store_spiffs_rollover(int max_messages, int keep_pct) {
    if (!s_initialized || max_messages <= 0 || s_active_count < max_messages) {
        return;
    }
    if (keep_pct < 50 || keep_pct > 90) {
        keep_pct = 75;
    }
    uint32_t keep_count = (uint32_t)((max_messages * keep_pct) / 100);
    if (keep_count > s_state.count) {
        keep_count = s_state.count;
    }
    mog_store_state_retain_recent(&s_state, keep_count);

    const uint64_t last_sequence = s_next_sequence > 0 ? s_next_sequence - 1 : 0;
    uint64_t new_generation = 0;
    if (mog_store_state_checkpoint(&s_state, SNAPSHOT_A, SNAPSHOT_B, JOURNAL_PATH,
                                   s_generation, last_sequence, &new_generation) !=
        MOG_STATE_OK) {
        ESP_LOGE(TAG, "checkpoint/rollover failed; committed old state remains authoritative");
        (void)recover_state();
        return;
    }

    s_generation = new_generation;
    s_active_count = (int)s_state.count;
    ESP_LOGI(TAG, "checkpoint complete: generation=%llu retained=%d",
             (unsigned long long)s_generation, s_active_count);
}

void msg_store_spiffs_clear(void) {
    unlink(SNAPSHOT_A);
    unlink(SNAPSHOT_B);
    unlink(JOURNAL_PATH);
    unlink(LEGACY_PATH);
    if (s_records) {
        memset(s_records, 0, CONFIG_BRAMBLE_MSG_PERSIST_MAX * sizeof(stored_msg_t));
    }
    mog_store_state_clear(&s_state);
    s_next_sequence = 1;
    s_generation = 0;
    s_active_count = 0;
    s_initialized = false;
    ESP_LOGI(TAG, "cleared transactional message store");
}

#else

int msg_store_spiffs_init(void) { return -1; }
void msg_store_spiffs_rollover(int max_messages, int keep_pct);

int msg_store_spiffs_save(const stored_msg_t *msg) { (void)msg; return -1; }
int msg_store_spiffs_update(int from_end, const stored_msg_t *msg) {
    (void)from_end; (void)msg; return -1;
}
int msg_store_spiffs_get_count(void) { return 0; }
int msg_store_spiffs_load_recent(stored_msg_t *msgs, int max_count) {
    (void)msgs; (void)max_count; return 0;
}
void msg_store_spiffs_rollover(int max_messages, int keep_pct) {
    (void)max_messages; (void)keep_pct;
}
void msg_store_spiffs_clear(void) {}

#endif
