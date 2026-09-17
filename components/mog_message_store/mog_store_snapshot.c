#include "mog_store_snapshot.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MOG_STORE_MAGIC 0x4D4F4753u       /* MOGS */
#define MOG_STORE_COMMIT_MAGIC 0x434D4954u /* CMIT */

typedef struct {
    uint32_t magic;
    uint16_t version;
    uint16_t header_size;
    uint32_t record_size;
    uint32_t record_count;
    uint64_t generation;
    uint32_t payload_crc32;
    uint32_t header_crc32;
    uint32_t commit_magic;
} mog_store_disk_header_t;

static uint32_t header_crc(mog_store_disk_header_t header) {
    header.header_crc32 = 0;
    return mog_store_crc32(&header, sizeof(header), 0);
}

uint32_t mog_store_crc32(const void *data, size_t len, uint32_t seed) {
    const uint8_t *p = (const uint8_t *)data;
    uint32_t crc = ~seed;

    for (size_t i = 0; i < len; ++i) {
        crc ^= p[i];
        for (unsigned bit = 0; bit < 8; ++bit) {
            const uint32_t mask = (uint32_t)-(int32_t)(crc & 1u);
            crc = (crc >> 1) ^ (0xEDB88320u & mask);
        }
    }
    return ~crc;
}

static int durable_flush(FILE *f) {
    if (fflush(f) != 0) {
        return MOG_STORE_ERR_IO;
    }
    if (fsync(fileno(f)) != 0) {
        return MOG_STORE_ERR_IO;
    }
    return MOG_STORE_OK;
}

static int checked_payload_size(uint32_t count, uint32_t record_size, size_t *out) {
    if (!out || record_size == 0) {
        return MOG_STORE_ERR_ARG;
    }
    if (count != 0 && (size_t)record_size > SIZE_MAX / (size_t)count) {
        return MOG_STORE_ERR_SIZE;
    }
    *out = (size_t)count * (size_t)record_size;
    return MOG_STORE_OK;
}

int mog_store_snapshot_write(const char *path,
                             uint64_t generation,
                             const void *records,
                             uint32_t record_count,
                             uint32_t record_size) {
    if (!path || record_size == 0 || (record_count > 0 && !records)) {
        return MOG_STORE_ERR_ARG;
    }

    size_t payload_size = 0;
    int rc = checked_payload_size(record_count, record_size, &payload_size);
    if (rc != MOG_STORE_OK) {
        return rc;
    }

    FILE *f = fopen(path, "w+b");
    if (!f) {
        return MOG_STORE_ERR_IO;
    }

    mog_store_disk_header_t header = {
        .magic = MOG_STORE_MAGIC,
        .version = MOG_STORE_SNAPSHOT_FORMAT_VERSION,
        .header_size = (uint16_t)sizeof(mog_store_disk_header_t),
        .record_size = record_size,
        .record_count = record_count,
        .generation = generation,
        .payload_crc32 = 0,
        .header_crc32 = 0,
        .commit_magic = 0,
    };

    /* Phase 1: uncommitted header plus the full payload. */
    if (fwrite(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return MOG_STORE_ERR_IO;
    }
    if (payload_size > 0 && fwrite(records, 1, payload_size, f) != payload_size) {
        fclose(f);
        return MOG_STORE_ERR_IO;
    }
    rc = durable_flush(f);
    if (rc != MOG_STORE_OK) {
        fclose(f);
        return rc;
    }

    /*
     * Phase 2: publish the commit only after the payload is durable. A power
     * cut before this header is durable leaves this slot invalid, while the
     * caller's previous slot remains a valid fallback.
     */
    header.payload_crc32 = mog_store_crc32(records, payload_size, 0);
    header.commit_magic = MOG_STORE_COMMIT_MAGIC;
    header.header_crc32 = header_crc(header);

    if (fseek(f, 0, SEEK_SET) != 0 || fwrite(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return MOG_STORE_ERR_IO;
    }

    rc = durable_flush(f);
    if (fclose(f) != 0 && rc == MOG_STORE_OK) {
        rc = MOG_STORE_ERR_IO;
    }
    return rc;
}

int mog_store_snapshot_validate(const char *path,
                                uint32_t expected_record_size,
                                mog_store_snapshot_info_t *out_info) {
    if (!path || expected_record_size == 0) {
        return MOG_STORE_ERR_ARG;
    }

    FILE *f = fopen(path, "rb");
    if (!f) {
        return MOG_STORE_ERR_IO;
    }

    mog_store_disk_header_t header;
    if (fread(&header, sizeof(header), 1, f) != 1) {
        fclose(f);
        return MOG_STORE_ERR_FORMAT;
    }

    if (header.magic != MOG_STORE_MAGIC ||
        header.version != MOG_STORE_SNAPSHOT_FORMAT_VERSION ||
        header.header_size != sizeof(header) ||
        header.commit_magic != MOG_STORE_COMMIT_MAGIC ||
        header.record_size != expected_record_size ||
        header.header_crc32 != header_crc(header)) {
        fclose(f);
        return MOG_STORE_ERR_FORMAT;
    }

    size_t payload_size = 0;
    int rc = checked_payload_size(header.record_count, header.record_size, &payload_size);
    if (rc != MOG_STORE_OK) {
        fclose(f);
        return rc;
    }
    if (payload_size > (size_t)LONG_MAX - sizeof(header)) {
        fclose(f);
        return MOG_STORE_ERR_SIZE;
    }

    struct stat st;
    if (fstat(fileno(f), &st) != 0 ||
        st.st_size != (off_t)(sizeof(header) + payload_size)) {
        fclose(f);
        return MOG_STORE_ERR_SIZE;
    }

    uint8_t buffer[512];
    size_t remaining = payload_size;
    uint32_t crc = 0;
    while (remaining > 0) {
        const size_t chunk = remaining < sizeof(buffer) ? remaining : sizeof(buffer);
        if (fread(buffer, 1, chunk, f) != chunk) {
            fclose(f);
            return MOG_STORE_ERR_IO;
        }
        crc = mog_store_crc32(buffer, chunk, crc);
        remaining -= chunk;
    }
    fclose(f);

    if (crc != header.payload_crc32) {
        return MOG_STORE_ERR_CRC;
    }

    if (out_info) {
        out_info->generation = header.generation;
        out_info->record_count = header.record_count;
        out_info->record_size = header.record_size;
        out_info->payload_crc32 = header.payload_crc32;
    }
    return MOG_STORE_OK;
}

int mog_store_snapshot_read(const char *path,
                            uint32_t expected_record_size,
                            void *records,
                            uint32_t records_capacity,
                            mog_store_snapshot_info_t *out_info) {
    mog_store_snapshot_info_t info = {0};
    int rc = mog_store_snapshot_validate(path, expected_record_size, &info);
    if (rc != MOG_STORE_OK) {
        return rc;
    }

    if (info.record_count > records_capacity) {
        return MOG_STORE_ERR_SIZE;
    }
    if (info.record_count > 0 && !records) {
        return MOG_STORE_ERR_ARG;
    }

    size_t payload_size = 0;
    rc = checked_payload_size(info.record_count, info.record_size, &payload_size);
    if (rc != MOG_STORE_OK) {
        return rc;
    }

    if (payload_size > 0) {
        FILE *f = fopen(path, "rb");
        if (!f) {
            return MOG_STORE_ERR_IO;
        }
        if (fseek(f, (long)sizeof(mog_store_disk_header_t), SEEK_SET) != 0 ||
            fread(records, 1, payload_size, f) != payload_size) {
            fclose(f);
            return MOG_STORE_ERR_IO;
        }
        if (fclose(f) != 0) {
            return MOG_STORE_ERR_IO;
        }

        /* Protect against a file being changed between validate() and read(). */
        if (mog_store_crc32(records, payload_size, 0) != info.payload_crc32) {
            return MOG_STORE_ERR_CRC;
        }
    }

    if (out_info) {
        *out_info = info;
    }
    return MOG_STORE_OK;
}

static int copy_path(char *out, size_t out_size, const char *path) {
    const size_t required = strlen(path) + 1;
    if (!out || out_size < required) {
        return MOG_STORE_ERR_SIZE;
    }
    memcpy(out, path, required);
    return MOG_STORE_OK;
}

int mog_store_snapshot_select(const char *slot_a,
                              const char *slot_b,
                              uint32_t expected_record_size,
                              char *out_path,
                              size_t out_path_size,
                              mog_store_snapshot_info_t *out_info) {
    if (!slot_a || !slot_b || !out_path || out_path_size == 0) {
        return MOG_STORE_ERR_ARG;
    }

    mog_store_snapshot_info_t a = {0};
    mog_store_snapshot_info_t b = {0};
    const int result_a = mog_store_snapshot_validate(slot_a, expected_record_size, &a);
    const int result_b = mog_store_snapshot_validate(slot_b, expected_record_size, &b);

    if (result_a != MOG_STORE_OK && result_b != MOG_STORE_OK) {
        return MOG_STORE_ERR_NO_VALID_SLOT;
    }

    const char *chosen_path;
    const mog_store_snapshot_info_t *chosen_info;
    if (result_a == MOG_STORE_OK &&
        (result_b != MOG_STORE_OK || a.generation >= b.generation)) {
        chosen_path = slot_a;
        chosen_info = &a;
    } else {
        chosen_path = slot_b;
        chosen_info = &b;
    }

    const int rc = copy_path(out_path, out_path_size, chosen_path);
    if (rc != MOG_STORE_OK) {
        return rc;
    }
    if (out_info) {
        *out_info = *chosen_info;
    }
    return MOG_STORE_OK;
}
