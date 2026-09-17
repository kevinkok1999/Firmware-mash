#include "msg_store_spiffs.h"

#include <assert.h>
#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>

#define TEST_STORE_DIR "/tmp/mog-spiffs"

static void ensure_dir(void) {
    if (mkdir(TEST_STORE_DIR, 0700) != 0) {
        assert(errno == EEXIST);
    }
}

static stored_msg_t make_msg(uint32_t uid, const char *text, uint32_t timestamp_s,
                             msg_status_t status) {
    stored_msg_t msg;
    memset(&msg, 0, sizeof(msg));
    msg.peer_addr = 0x1000u + uid;
    msg.uid = uid;
    msg.direction = MSG_DIR_OUTGOING;
    msg.status = status;
    msg.packet_id = uid * 10u;
    msg.timestamp_s = timestamp_s;
    msg.channel_index = -1;
    msg.text_len = (uint16_t)strlen(text);
    assert(msg.text_len < MSG_TEXT_MAX);
    memcpy(msg.text, text, msg.text_len + 1u);
    return msg;
}

static int load_all(stored_msg_t *out, int cap) {
    memset(out, 0, (size_t)cap * sizeof(*out));
    return msg_store_spiffs_load_recent(out, cap);
}

static void mode_write_timestamp(void) {
    assert(msg_store_spiffs_init() == 0);
    msg_store_spiffs_clear();

    stored_msg_t msg = make_msg(1, "persist-me", 123u, MSG_STATUS_SENT);
    assert(msg_store_spiffs_save(&msg) == 0);

    /* Simulate Bramble after reboot semantics: RAM timestamp is zero, but a
     * status update must preserve the original durable timestamp. */
    stored_msg_t updated = msg;
    updated.status = MSG_STATUS_DELIVERED;
    updated.timestamp_s = 0;
    assert(msg_store_spiffs_update(0, &updated) == 0);

    stored_msg_t loaded[2];
    assert(load_all(loaded, 2) == 1);
    assert(loaded[0].uid == 1);
    assert(loaded[0].status == MSG_STATUS_DELIVERED);
    assert(loaded[0].timestamp_s == 123u);
}

static void mode_read_timestamp(void) {
    assert(msg_store_spiffs_init() == 0);
    stored_msg_t loaded[2];
    assert(load_all(loaded, 2) == 1);
    assert(loaded[0].uid == 1);
    assert(loaded[0].status == MSG_STATUS_DELIVERED);
    assert(loaded[0].timestamp_s == 123u);
}

static void mode_fill(void) {
    assert(msg_store_spiffs_init() == 0);
    assert(msg_store_spiffs_get_count() == 1);

    for (uint32_t uid = 2; uid <= 8; ++uid) {
        char text[24];
        snprintf(text, sizeof(text), "message-%u", (unsigned)uid);
        stored_msg_t msg = make_msg(uid, text, 100u + uid, MSG_STATUS_SENT);
        assert(msg_store_spiffs_save(&msg) == 0);
    }
    assert(msg_store_spiffs_get_count() == 8);

    /* Capacity is 8 in the test sdkconfig. The ninth save must proactively
     * compact to 75% before appending instead of deadlocking at a full store. */
    stored_msg_t ninth = make_msg(9, "message-9", 109u, MSG_STATUS_SENT);
    assert(msg_store_spiffs_save(&ninth) == 0);
    assert(msg_store_spiffs_get_count() == 7);

    stored_msg_t loaded[8];
    const int count = load_all(loaded, 8);
    assert(count == 7);
    assert(loaded[0].uid == 3);
    assert(loaded[count - 1].uid == 9);
}

static void mode_read_fill(void) {
    assert(msg_store_spiffs_init() == 0);
    stored_msg_t loaded[8];
    const int count = load_all(loaded, 8);
    assert(count == 7);
    assert(loaded[0].uid == 3);
    assert(loaded[count - 1].uid == 9);
}

static void mode_torn_setup(void) {
    assert(msg_store_spiffs_init() == 0);
    msg_store_spiffs_clear();
    stored_msg_t first = make_msg(11, "first", 211u, MSG_STATUS_SENT);
    stored_msg_t second = make_msg(12, "second", 212u, MSG_STATUS_SENT);
    assert(msg_store_spiffs_save(&first) == 0);
    assert(msg_store_spiffs_save(&second) == 0);
    assert(msg_store_spiffs_get_count() == 2);
}

static void mode_torn_read_and_append(void) {
    /* Recovery must expose the valid prefix, repair the torn tail before any
     * append, then allow a new committed message to follow safely. */
    assert(msg_store_spiffs_init() == 0);
    stored_msg_t loaded[4];
    int count = load_all(loaded, 4);
    assert(count == 1);
    assert(loaded[0].uid == 11);

    stored_msg_t third = make_msg(13, "after-repair", 213u, MSG_STATUS_SENT);
    assert(msg_store_spiffs_save(&third) == 0);
    count = load_all(loaded, 4);
    assert(count == 2);
    assert(loaded[0].uid == 11);
    assert(loaded[1].uid == 13);
}

static void mode_torn_verify(void) {
    assert(msg_store_spiffs_init() == 0);
    stored_msg_t loaded[4];
    const int count = load_all(loaded, 4);
    assert(count == 2);
    assert(loaded[0].uid == 11);
    assert(loaded[1].uid == 13);
}

static void mode_clean(void) {
    assert(msg_store_spiffs_init() == 0);
    msg_store_spiffs_clear();
}

int main(int argc, char **argv) {
    assert(argc == 2);
    ensure_dir();

    if (strcmp(argv[1], "write-timestamp") == 0) {
        mode_write_timestamp();
    } else if (strcmp(argv[1], "read-timestamp") == 0) {
        mode_read_timestamp();
    } else if (strcmp(argv[1], "fill") == 0) {
        mode_fill();
    } else if (strcmp(argv[1], "read-fill") == 0) {
        mode_read_fill();
    } else if (strcmp(argv[1], "torn-setup") == 0) {
        mode_torn_setup();
    } else if (strcmp(argv[1], "torn-read-append") == 0) {
        mode_torn_read_and_append();
    } else if (strcmp(argv[1], "torn-verify") == 0) {
        mode_torn_verify();
    } else if (strcmp(argv[1], "clean") == 0) {
        mode_clean();
    } else {
        fprintf(stderr, "unknown mode: %s\n", argv[1]);
        return 2;
    }

    printf("test_mog_bramble_adapter(%s): PASS\n", argv[1]);
    return 0;
}
