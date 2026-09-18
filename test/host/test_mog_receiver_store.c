#include "mog_receiver_store.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *journal = "/tmp/mog-receiver-store-test.bin";

int main(void) {
    unlink(journal);
    mog_receiver_record_t records[4];
    memset(records, 0, sizeof(records));
    mog_receiver_store_t store;
    assert(mog_receiver_store_init(&store, records, 4, journal, 1) == MOG_RECEIVER_STORE_OK);

    const mog_message_key_t a = {.origin = 7, .packet_id = 0x100000001ULL};
    const mog_message_key_t same_packet_other_origin = {.origin = 8, .packet_id = 0x100000001ULL};
    const mog_message_key_t high_bits_differ = {.origin = 7, .packet_id = 0x200000001ULL};

    assert(mog_receiver_store_mark_received(&store, a) == MOG_RECEIVER_STORE_OK);
    assert(mog_receiver_store_mark_received(&store, a) == MOG_RECEIVER_STORE_OK);
    assert(store.state.count == 1);
    assert(mog_receiver_store_mark_received(&store, same_packet_other_origin) == MOG_RECEIVER_STORE_OK);
    assert(mog_receiver_store_mark_received(&store, high_bits_differ) == MOG_RECEIVER_STORE_OK);
    assert(store.state.count == 3);

    const mog_receiver_record_t *ra = mog_receiver_store_find(&store, a);
    assert(ra && ra->origin == a.origin && ra->packet_id == a.packet_id);
    assert((ra->flags & MOG_RECEIVER_FLAG_PRESENTED) == 0);
    assert(mog_receiver_store_mark_presented(&store, a) == MOG_RECEIVER_STORE_OK);
    ra = mog_receiver_store_find(&store, a);
    assert(ra && (ra->flags & MOG_RECEIVER_FLAG_PRESENTED) != 0);
    assert(mog_receiver_store_mark_presented(&store, a) == MOG_RECEIVER_STORE_OK);

    /* Reconstruct state from the durable journal and validate full identity. */
    mog_receiver_record_t recovered[4];
    memset(recovered, 0, sizeof(recovered));
    mog_receiver_store_t rebooted;
    assert(mog_receiver_store_init(&rebooted, recovered, 4, journal, 1) == MOG_RECEIVER_STORE_OK);
    unsigned char scratch[sizeof(mog_receiver_record_t)];
    mog_store_recovery_info_t info;
    unlink("/tmp/mog-rx-a.bin");
    unlink("/tmp/mog-rx-b.bin");
    assert(mog_store_state_recover(&rebooted.state, "/tmp/mog-rx-a.bin", "/tmp/mog-rx-b.bin",
                                   journal, scratch, sizeof(scratch), &info) == MOG_STATE_OK);
    assert(mog_receiver_store_finish_recovery(&rebooted, info.next_sequence) == MOG_RECEIVER_STORE_OK);
    assert(rebooted.state.count == 3);
    ra = mog_receiver_store_find(&rebooted, a);
    assert(ra && (ra->flags & MOG_RECEIVER_FLAG_PRESENTED) != 0);
    assert(mog_receiver_store_find(&rebooted, same_packet_other_origin));
    assert(mog_receiver_store_find(&rebooted, high_bits_differ));

    const mog_message_key_t fourth = {.origin = 9, .packet_id = 4};
    const mog_message_key_t fifth = {.origin = 10, .packet_id = 5};
    assert(mog_receiver_store_mark_received(&rebooted, fourth) == MOG_RECEIVER_STORE_OK);
    assert(mog_receiver_store_mark_received(&rebooted, fifth) == MOG_RECEIVER_STORE_ERR_FULL);

    unlink(journal);
    puts("mog_receiver_store tests: PASS");
    return 0;
}
