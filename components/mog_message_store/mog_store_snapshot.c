#include "mog_store_snapshot.h"

#include <limits.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

#define MOG_STORE_HEADER_SIZE 48u
#define OFF_MAGIC 0u
#define OFF_VERSION 4u
#define OFF_HEADER_SIZE 6u
#define OFF_RECORD_SIZE 8u
#define OFF_RECORD_COUNT 12u
#define OFF_GENERATION 16u
#define OFF_LAST_SEQUENCE 24u
#define OFF_PAYLOAD_CRC 32u
#define OFF_HEADER_CRC 36u
#define OFF_COMMIT 40u
#define OFF_RESERVED 44u

static const uint8_t k_magic[4] = {'M', 'O', 'G', 'S'};
static const uint8_t k_commit[4] = {'C', 'M', 'I', 'T'};

static void put_u16_le(uint8_t *p, uint16_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
}

static void put_u32_le(uint8_t *p, uint32_t v) {
    p[0] = (uint8_t)(v & 0xffu);
    p[1] = (uint8_t)((v >> 8) & 0xffu);
    p[2] = (uint8_t)((v >> 16) & 0xffu);
    p[3] = (uint8_t)((v >> 24) & 0xffu);
}

static void put_u64_le(uint8_t *p, uint64_t v) {
    for (unsigned i = 0; i < 8; ++i) {
        p[i] = (uint8_t)((v >> (8u * i)) & 0xffu);
    }
}

static uint16_t get_u16_le(const uint8_t *p) {
    return (uint16_t)p[0] | (uint16_t)((uint16_t)p[1] << 8);
}

static uint32_t get_u32_le(const uint8_t *p) {
    return (uint32_t)p[0] |
           ((uint32_t)p[1] << 8) |
           ((uint32_t)p[2] << 16) |
           ((uint32_t)p[3] << 24);
}

static uint64_t get_u64_le(const uint8_t *p) {
    uint64_t v = 0;
    for (unsigned i = 0; i < 8; ++i) {
        v |= (uint64_t)p[i] << (8u * i);
    }
    return v;
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

static uint32_t header_crc(const uint8_t header[MOG_STORE_HEADER_SIZE]) {
    uint8_t copy[MOG_STORE_HEADER_SIZE];
    memcpy(copy, header, sizeof(copy));
    memset(copy + OFF_HEADER_CRC, 0, 4);
    return mog_store_crc32(copy, sizeof(copy), 0);
}

static int durable_flush(FILE *f) {
    if (fflush(f) != 0 || fsync(fileno(f)) != 0) {
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

static void build_header(uint8_t header[MOG_STORE_HEADER_SIZE],
                         uint64_t generation,
                         uint64_t last_sequence,
                         uint32_t record_count,
                         uint32_t record_size) {
    memset(header, 0, MOG_STORE_HEADER_SIZE);
    memcpy(header + OFF_MAGIC, k_magic, sizeof(k_magic));
    put_u16_le(header + OFF_VERSION, MOG_STORE_SNAPSHOT_FORMAT_VERSION);
    put_u16_le(header + OFF_HEADER_SIZE, MOG_STORE_HEADER_SIZE);
    put_u32_le(header + OFF_RECORD_SIZE, record_size);
    put_u32_le(header + OFF_RECORD_COUNT, record_count);
    put_u64_le(header + OFF_GENERATION, generation);
    put_u64_le(header + OFF_LAST_SEQUENCE, last_sequence);
}

int mog_store_snapshot_write(const char *path,
                             uint64_t generation,
                             uint64_t last_sequence,
                             const void *records,
                             uint32_t record_count,
                             uint32_t record_size) {
    if (!path || generation == 0 || record_size == 0 ||
        (record_count > 0 && !records)) {
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

    uint8_t header[MOG_STORE_HEADER_SIZE];
    build_header(header, generation, last_sequence, record_count, record_size);
    if (fwrite(header, 1, sizeof(header), f) != sizeof(header) ||
        (payload_size > 0 && fwrite(records, 1, payload_size, f) != payload_size)) {
        fclose(f);
        return MOG_STORE_ERR_IO;
    }
    rc = durable_flush(f);
    if (rc != MOG_STORE_OK) {
        fclose(f);
        return rc;
    }

    put_u32_le(header + OFF_PAYLOAD_CRC, mog_store_crc32(records, payload_size, 0));
    memcpy(header + OFF_COMMIT, k_commit, sizeof(k_commit));
    put_u32_le(header + OFF_HEADER_CRC, header_crc(header));

    if (fseek(f, 0, SEEK_SET) != 0 || fwrite(header, 1, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        return MOG_STORE_ERR_IO;
    }

    rc = durable_flush(f);
    if (fclose(f) != 0 && rc == MOG_STORE_OK) {
        rc = MOG_STORE_ERR_IO;
    }
    return rc;
}

static int decode_header(const uint8_t header[MOG_STORE_HEADER_SIZE],
                         uint32_t expected_record_size,
                         mog_store_snapshot_info_t *info) {
    if (memcmp(header + OFF_MAGIC, k_magic, sizeof(k_magic)) != 0 ||
        get_u16_le(header + OFF_VERSION) != MOG_STORE_SNAPSHOT_FORMAT_VERSION ||
        get_u16_le(header + OFF_HEADER_SIZE) != MOG_STORE_HEADER_SIZE ||
        memcmp(header + OFF_COMMIT, k_commit, sizeof(k_commit)) != 0 ||
        get_u32_le(header + OFF_RESERVED) != 0 ||
        get_u32_le(header + OFF_RECORD_SIZE) != expected_record_size ||
        get_u32_le(header + OFF_HEADER_CRC) != header_crc(header)) {
        return MOG_STORE_ERR_FORMAT;
    }

    info->record_size = get_u32_le(header + OFF_RECORD_SIZE);
    info->record_count = get_u32_le(header + OFF_RECORD_COUNT);
    info->generation = get_u64_le(header + OFF_GENERATION);
    info->last_sequence = get_u64_le(header + OFF_LAST_SEQUENCE);
    info->payload_crc32 = get_u32_le(header + OFF_PAYLOAD_CRC);
    if (info->generation == 0) {
        return MOG_STORE_ERR_FORMAT;
    }
    return MOG_STORE_OK;
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

    uint8_t header[MOG_STORE_HEADER_SIZE];
    if (fread(header, 1, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        return MOG_STORE_ERR_FORMAT;
    }

    mog_store_snapshot_info_t info = {0};
    int rc = decode_header(header, expected_record_size, &info);
    if (rc != MOG_STORE_OK) {
        fclose(f);
        return rc;
    }

    size_t payload_size = 0;
    rc = checked_payload_size(info.record_count, info.record_size, &payload_size);
    if (rc != MOG_STORE_OK) {
        fclose(f);
        return rc;
    }
    if (payload_size > (size_t)LONG_MAX - MOG_STORE_HEADER_SIZE) {
        fclose(f);
        return MOG_STORE_ERR_SIZE;
    }

    struct stat st;
    if (fstat(fileno(f), &st) != 0 ||
        st.st_size != (off_t)(MOG_STORE_HEADER_SIZE + payload_size)) {
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

    if (fclose(f) != 0) {
        return MOG_STORE_ERR_IO;
    }
    if (crc != info.payload_crc32) {
        return MOG_STORE_ERR_CRC;
    }

    if (out_info) {
        *out_info = info;
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
        if (fseek(f, MOG_STORE_HEADER_SIZE, SEEK_SET) != 0 ||
            fread(records, 1, payload_size, f) != payload_size) {
            fclose(f);
            return MOG_STORE_ERR_IO;
        }
        if (fclose(f) != 0) {
            return MOG_STORE_ERR_IO;
        }
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
        (result_b != MOG_STORE_OK || a.generation > b.generation ||
         (a.generation == b.generation && a.last_sequence >= b.last_sequence))) {
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
