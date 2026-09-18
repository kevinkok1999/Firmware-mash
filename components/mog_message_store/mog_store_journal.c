#include "mog_store_journal.h"
#include "mog_store_snapshot.h"

#include <errno.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#define MOG_JOURNAL_MAGIC 0x4d4f474au       /* MOGJ */
#define MOG_JOURNAL_COMMIT_MAGIC 0x434d4954u /* CMIT */
#define MOG_JOURNAL_HEADER_SIZE 40u

#define OFF_MAGIC 0u
#define OFF_VERSION 4u
#define OFF_HEADER_SIZE 6u
#define OFF_OP 8u
#define OFF_RESERVED 10u
#define OFF_PAYLOAD_SIZE 12u
#define OFF_SEQUENCE 16u
#define OFF_UID 24u
#define OFF_PAYLOAD_CRC 28u
#define OFF_HEADER_CRC 32u
#define OFF_COMMIT 36u

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

static uint32_t journal_header_crc(uint8_t header[MOG_JOURNAL_HEADER_SIZE]) {
    uint8_t copy[MOG_JOURNAL_HEADER_SIZE];
    memcpy(copy, header, sizeof(copy));
    memset(copy + OFF_HEADER_CRC, 0, sizeof(uint32_t));
    return mog_store_crc32(copy, sizeof(copy), 0);
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

static int operation_is_valid(mog_store_journal_op_t op) {
    return op == MOG_STORE_JOURNAL_PUT || op == MOG_STORE_JOURNAL_DELETE;
}

static int payload_contract_is_valid(mog_store_journal_op_t op,
                                     const void *payload,
                                     uint32_t payload_size,
                                     uint32_t record_size) {
    if (record_size == 0) {
        return 0;
    }
    if (op == MOG_STORE_JOURNAL_PUT) {
        return payload != NULL && payload_size == record_size;
    }
    if (op == MOG_STORE_JOURNAL_DELETE) {
        return payload == NULL && payload_size == 0;
    }
    return 0;
}

int mog_store_journal_append(const char *path,
                             mog_store_journal_op_t op,
                             uint64_t sequence,
                             uint32_t uid,
                             const void *payload,
                             uint32_t payload_size,
                             uint32_t record_size) {
    if (!path || sequence == 0 || uid == 0 || !operation_is_valid(op) ||
        !payload_contract_is_valid(op, payload, payload_size, record_size)) {
        return MOG_JOURNAL_ERR_ARG;
    }

    FILE *f = fopen(path, "r+b");
    if (!f) {
        f = fopen(path, "w+b");
    }
    if (!f) {
        return MOG_JOURNAL_ERR_IO;
    }
    if (fseek(f, 0, SEEK_END) != 0) {
        fclose(f);
        return MOG_JOURNAL_ERR_IO;
    }
    const long entry_offset = ftell(f);
    if (entry_offset < 0) {
        fclose(f);
        return MOG_JOURNAL_ERR_IO;
    }

    uint8_t header[MOG_JOURNAL_HEADER_SIZE] = {0};
    put_u32_le(header + OFF_MAGIC, MOG_JOURNAL_MAGIC);
    put_u16_le(header + OFF_VERSION, MOG_STORE_JOURNAL_FORMAT_VERSION);
    put_u16_le(header + OFF_HEADER_SIZE, MOG_JOURNAL_HEADER_SIZE);
    put_u16_le(header + OFF_OP, (uint16_t)op);
    put_u16_le(header + OFF_RESERVED, 0);
    put_u32_le(header + OFF_PAYLOAD_SIZE, payload_size);
    put_u64_le(header + OFF_SEQUENCE, sequence);
    put_u32_le(header + OFF_UID, uid);

    if (fwrite(header, 1, sizeof(header), f) != sizeof(header) ||
        (payload_size > 0 && fwrite(payload, 1, payload_size, f) != payload_size)) {
        fclose(f);
        return MOG_JOURNAL_ERR_IO;
    }
    int rc = durable_flush(f);
    if (rc != MOG_JOURNAL_OK) {
        fclose(f);
        return rc;
    }

    put_u32_le(header + OFF_PAYLOAD_CRC, mog_store_crc32(payload, payload_size, 0));
    put_u32_le(header + OFF_COMMIT, MOG_JOURNAL_COMMIT_MAGIC);
    put_u32_le(header + OFF_HEADER_CRC, journal_header_crc(header));

    if (fseek(f, entry_offset, SEEK_SET) != 0 ||
        fwrite(header, 1, sizeof(header), f) != sizeof(header)) {
        fclose(f);
        return MOG_JOURNAL_ERR_IO;
    }
    rc = durable_flush(f);
    if (fclose(f) != 0 && rc == MOG_JOURNAL_OK) {
        rc = MOG_JOURNAL_ERR_IO;
    }
    return rc;
}

static int decode_and_validate_header(uint8_t header[MOG_JOURNAL_HEADER_SIZE],
                                      uint32_t record_size,
                                      mog_store_journal_op_t *out_op,
                                      uint64_t *out_sequence,
                                      uint32_t *out_uid,
                                      uint32_t *out_payload_size,
                                      uint32_t *out_payload_crc) {
    if (get_u32_le(header + OFF_MAGIC) != MOG_JOURNAL_MAGIC ||
        get_u16_le(header + OFF_VERSION) != MOG_STORE_JOURNAL_FORMAT_VERSION ||
        get_u16_le(header + OFF_HEADER_SIZE) != MOG_JOURNAL_HEADER_SIZE ||
        get_u16_le(header + OFF_RESERVED) != 0 ||
        get_u32_le(header + OFF_COMMIT) != MOG_JOURNAL_COMMIT_MAGIC ||
        get_u32_le(header + OFF_HEADER_CRC) != journal_header_crc(header)) {
        return MOG_JOURNAL_ERR_FORMAT;
    }

    const mog_store_journal_op_t op =
        (mog_store_journal_op_t)get_u16_le(header + OFF_OP);
    const uint32_t payload_size = get_u32_le(header + OFF_PAYLOAD_SIZE);
    const uint64_t sequence = get_u64_le(header + OFF_SEQUENCE);
    const uint32_t uid = get_u32_le(header + OFF_UID);

    if (!operation_is_valid(op) || sequence == 0 || uid == 0) {
        return MOG_JOURNAL_ERR_FORMAT;
    }
    if ((op == MOG_STORE_JOURNAL_PUT && payload_size != record_size) ||
        (op == MOG_STORE_JOURNAL_DELETE && payload_size != 0)) {
        return MOG_JOURNAL_ERR_FORMAT;
    }

    *out_op = op;
    *out_sequence = sequence;
    *out_uid = uid;
    *out_payload_size = payload_size;
    *out_payload_crc = get_u32_le(header + OFF_PAYLOAD_CRC);
    return MOG_JOURNAL_OK;
}

int mog_store_journal_replay(const char *path,
                             uint32_t record_size,
                             uint64_t min_sequence_exclusive,
                             void *scratch,
                             size_t scratch_size,
                             mog_store_journal_replay_cb callback,
                             void *ctx,
                             mog_store_journal_scan_info_t *out_info) {
    if (!path || record_size == 0 || !scratch || scratch_size < record_size) {
        return MOG_JOURNAL_ERR_ARG;
    }

    mog_store_journal_scan_info_t info = {0};
    FILE *f = fopen(path, "rb");
    if (!f) {
        if (errno == ENOENT) {
            if (out_info) {
                *out_info = info;
            }
            return MOG_JOURNAL_OK;
        }
        return MOG_JOURNAL_ERR_IO;
    }

    uint64_t previous_sequence = 0;
    for (;;) {
        uint8_t header[MOG_JOURNAL_HEADER_SIZE];
        const size_t got_header = fread(header, 1, sizeof(header), f);
        if (got_header == 0) {
            if (feof(f)) {
                break;
            }
            fclose(f);
            return MOG_JOURNAL_ERR_IO;
        }
        if (got_header != sizeof(header)) {
            info.recovered_partial = 1;
            clearerr(f);
            break;
        }

        mog_store_journal_op_t op;
        uint64_t sequence;
        uint32_t uid;
        uint32_t payload_size;
        uint32_t payload_crc;
        const int header_rc = decode_and_validate_header(header, record_size, &op, &sequence,
                                                         &uid, &payload_size, &payload_crc);
        if (header_rc != MOG_JOURNAL_OK || sequence <= previous_sequence) {
            info.recovered_partial = 1;
            break;
        }

        if (payload_size > scratch_size) {
            fclose(f);
            return MOG_JOURNAL_ERR_SIZE;
        }
        if (payload_size > 0 && fread(scratch, 1, payload_size, f) != payload_size) {
            info.recovered_partial = 1;
            clearerr(f);
            break;
        }
        if (mog_store_crc32(scratch, payload_size, 0) != payload_crc) {
            info.recovered_partial = 1;
            break;
        }

        previous_sequence = sequence;
        info.valid_entries++;
        info.max_sequence = sequence;
        const long end = ftell(f);
        if (end < 0) {
            fclose(f);
            return MOG_JOURNAL_ERR_IO;
        }
        info.valid_bytes = (size_t)end;

        if (sequence > min_sequence_exclusive && callback) {
            if (callback(op, sequence, uid,
                         payload_size > 0 ? scratch : NULL,
                         payload_size, ctx) != 0) {
                fclose(f);
                return MOG_JOURNAL_ERR_CALLBACK;
            }
        }
    }

    if (fclose(f) != 0) {
        return MOG_JOURNAL_ERR_IO;
    }
    if (out_info) {
        *out_info = info;
    }
    return info.recovered_partial ? MOG_JOURNAL_RECOVERED_PARTIAL : MOG_JOURNAL_OK;
}

int mog_store_journal_truncate(const char *path, size_t valid_bytes) {
    if (!path) {
        return MOG_JOURNAL_ERR_ARG;
    }
    FILE *f = fopen(path, "r+b");
    if (!f) {
        return errno == ENOENT && valid_bytes == 0 ? MOG_JOURNAL_OK : MOG_JOURNAL_ERR_IO;
    }
    if (ftruncate(fileno(f), (off_t)valid_bytes) != 0) {
        fclose(f);
        return MOG_JOURNAL_ERR_IO;
    }
    const int rc = durable_flush(f);
    if (fclose(f) != 0 && rc == MOG_JOURNAL_OK) {
        return MOG_JOURNAL_ERR_IO;
    }
    return rc;
}
