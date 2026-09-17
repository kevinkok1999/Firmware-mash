#include "mog_store_journal.h"
#include "mog_store_snapshot.h"

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define MOG_JOURNAL_MAGIC 0x4D4F474Au /* MOGJ */

typedef struct __attribute__((packed)) {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t op;
    uint32_t key;
    uint32_t payload_len;
    uint64_t sequence;
    uint32_t payload_crc32;
    uint32_t header_crc32;
} mog_store_journal_disk_header_t;

_Static_assert(sizeof(mog_store_journal_disk_header_t) == 36,
               "journal header layout changed; bump format version deliberately");

static uint32_t journal_header_crc(mog_store_journal_disk_header_t header) {
    header.header_crc32 = 0;
    return mog_store_crc32(&header, sizeof(header), 0);
}

static int durable_flush(FILE *f) {
    if (fflush(f) != 0) {
        return MOG_JOURNAL_ERR_IO;
    }
    if (fsync(fileno(f)) != 0) {
        return MOG_JOURNAL_ERR_IO;
    }
    return MOG_JOURNAL_OK;
}

static bool valid_op(uint32_t op) {
    return op == MOG_JOURNAL_OP_UPSERT || op == MOG_JOURNAL_OP_DELETE;
}

int mog_store_journal_append(const char *path,
                             uint64_t sequence,
                             uint32_t op,
                             uint32_t key,
                             const void *payload,
                             uint32_t payload_len) {
    if (!path || sequence == 0 || !valid_op(op) || (payload_len > 0 && !payload)) {
        return MOG_JOURNAL_ERR_ARG;
    }
    if (op == MOG_JOURNAL_OP_DELETE && payload_len != 0) {
        return MOG_JOURNAL_ERR_ARG;
    }

    mog_store_journal_disk_header_t header = {
        .magic = MOG_JOURNAL_MAGIC,
        .version = MOG_STORE_JOURNAL_FORMAT_VERSION,
        .header_size = (uint16_t)sizeof(mog_store_journal_disk_header_t),
        .op = op,
        .key = key,
        .payload_len = payload_len,
        .sequence = sequence,
        .payload_crc32 = mog_store_crc32(payload, payload_len, 0),
        .header_crc32 = 0,
    };
    header.header_crc32 = journal_header_crc(header);

    FILE *f = fopen(path, "ab");
    if (!f) {
        return MOG_JOURNAL_ERR_IO;
    }

    int rc = MOG_JOURNAL_OK;
    if (fwrite(&header, sizeof(header), 1, f) != 1 ||
        (payload_len > 0 && fwrite(payload, 1, payload_len, f) != payload_len)) {
        rc = MOG_JOURNAL_ERR_IO;
    } else {
        rc = durable_flush(f);
    }

    if (fclose(f) != 0 && rc == MOG_JOURNAL_OK) {
        rc = MOG_JOURNAL_ERR_IO;
    }
    return rc;
}

typedef struct {
    mog_store_journal_replay_fn callback;
    void *ctx;
    uint32_t max_payload_len;
    mog_store_journal_scan_info_t info;
} scan_ctx_t;

static int scan_journal(const char *path, scan_ctx_t *scan) {
    FILE *f = fopen(path, "rb");
    if (!f) {
        return MOG_JOURNAL_ERR_IO;
    }

    memset(&scan->info, 0, sizeof(scan->info));

    for (;;) {
        const long entry_start = ftell(f);
        if (entry_start < 0) {
            fclose(f);
            return MOG_JOURNAL_ERR_IO;
        }

        mog_store_journal_disk_header_t header;
        const size_t header_read = fread(&header, 1, sizeof(header), f);
        if (header_read == 0 && feof(f)) {
            break; /* exact clean EOF */
        }
        if (header_read != sizeof(header)) {
            scan->info.tail_damaged = true;
            break;
        }

        const bool header_valid =
            header.magic == MOG_JOURNAL_MAGIC &&
            header.version == MOG_STORE_JOURNAL_FORMAT_VERSION &&
            header.header_size == sizeof(header) &&
            valid_op(header.op) &&
            header.header_crc32 == journal_header_crc(header) &&
            header.payload_len <= scan->max_payload_len &&
            header.sequence > scan->info.last_sequence &&
            !(header.op == MOG_JOURNAL_OP_DELETE && header.payload_len != 0);

        if (!header_valid) {
            scan->info.tail_damaged = true;
            break;
        }

        uint8_t *payload = NULL;
        if (header.payload_len > 0) {
            payload = (uint8_t *)malloc(header.payload_len);
            if (!payload) {
                fclose(f);
                return MOG_JOURNAL_ERR_IO;
            }
            if (fread(payload, 1, header.payload_len, f) != header.payload_len) {
                free(payload);
                scan->info.tail_damaged = true;
                break;
            }
            if (mog_store_crc32(payload, header.payload_len, 0) != header.payload_crc32) {
                free(payload);
                scan->info.tail_damaged = true;
                break;
            }
        } else if (header.payload_crc32 != mog_store_crc32(NULL, 0, 0)) {
            scan->info.tail_damaged = true;
            break;
        }

        if (scan->callback) {
            const mog_store_journal_entry_t entry = {
                .sequence = header.sequence,
                .op = header.op,
                .key = header.key,
                .payload = payload,
                .payload_len = header.payload_len,
            };
            const int callback_rc = scan->callback(&entry, scan->ctx);
            if (callback_rc != 0) {
                free(payload);
                fclose(f);
                return callback_rc;
            }
        }
        free(payload);

        const long entry_end = ftell(f);
        if (entry_end < entry_start) {
            fclose(f);
            return MOG_JOURNAL_ERR_IO;
        }
        scan->info.last_sequence = header.sequence;
        scan->info.entry_count++;
        scan->info.valid_bytes = (uint64_t)entry_end;
    }

    if (fclose(f) != 0) {
        return MOG_JOURNAL_ERR_IO;
    }
    return MOG_JOURNAL_OK;
}

int mog_store_journal_replay(const char *path,
                             uint32_t max_payload_len,
                             mog_store_journal_replay_fn callback,
                             void *ctx,
                             mog_store_journal_scan_info_t *out_info) {
    if (!path || max_payload_len == 0) {
        return MOG_JOURNAL_ERR_ARG;
    }

    scan_ctx_t scan = {
        .callback = callback,
        .ctx = ctx,
        .max_payload_len = max_payload_len,
        .info = {0},
    };
    const int rc = scan_journal(path, &scan);
    if (out_info) {
        *out_info = scan.info;
    }
    return rc;
}

int mog_store_journal_repair_tail(const char *path,
                                  uint32_t max_payload_len,
                                  mog_store_journal_scan_info_t *out_info) {
    if (!path || max_payload_len == 0) {
        return MOG_JOURNAL_ERR_ARG;
    }

    mog_store_journal_scan_info_t info = {0};
    int rc = mog_store_journal_replay(path, max_payload_len, NULL, NULL, &info);
    if (rc != MOG_JOURNAL_OK) {
        return rc;
    }

    if (info.tail_damaged) {
        FILE *f = fopen(path, "r+b");
        if (!f) {
            return MOG_JOURNAL_ERR_IO;
        }
        if (ftruncate(fileno(f), (off_t)info.valid_bytes) != 0) {
            fclose(f);
            return MOG_JOURNAL_ERR_IO;
        }
        rc = durable_flush(f);
        if (fclose(f) != 0 && rc == MOG_JOURNAL_OK) {
            rc = MOG_JOURNAL_ERR_IO;
        }
        if (rc != MOG_JOURNAL_OK) {
            return rc;
        }
        info.tail_damaged = false;
    }

    if (out_info) {
        *out_info = info;
    }
    return MOG_JOURNAL_OK;
}
