#include "mog_receive_flow.h"
#include "mog_store_state.h"

#include <assert.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

static const char *journal = "/tmp/mog-receive-flow-test.bin";
static const char *snap_a = "/tmp/mog-receive-flow-a.bin";
static const char *snap_b = "/tmp/mog-receive-flow-b.bin";

static void recover_store(mog_receiver_store_t *store,
                          mog_receiver_record_t *records,
                          uint32_t capacity)
{
    unsigned char scratch[sizeof(mog_receiver_record_t)];
    mog_store_recovery_info_t info;
    memset(records, 0, sizeof(*records) * capacity);
    assert(mog_receiver_store_init(store, records, capacity, journal, 1) ==
           MOG_RECEIVER_STORE_OK);
    assert(mog_store_state_recover(&store->state, snap_a, snap_b, journal,
                                   scratch, sizeof(scratch), &info) == MOG_STATE_OK);
    assert(mog_receiver_store_finish_recovery(store, info.next_sequence) ==
           MOG_RECEIVER_STORE_OK);
}

int main(void)
{
    unlink(journal);
    unlink(snap_a);
    unlink(snap_b);

    mog_receiver_record_t records[8];
    memset(records, 0, sizeof(records));
    mog_receiver_store_t store;
    mog_dedup_t dedup;
    assert(mog_receiver_store_init(&store, records, 8, journal, 1) ==
           MOG_RECEIVER_STORE_OK);
    assert(mog_receive_flow_restore(&dedup, &store) == MOG_RECEIVE_FLOW_OK);

    const mog_message_key_t key = {.origin = 42, .packet_id = 0x100000009ULL};
    bool present = false, ack = false;
    assert(mog_receive_flow_receive(&store, &dedup, key, 100, &present, &ack) ==
           MOG_RECEIVE_FLOW_OK);
    assert(present && ack);
    assert(mog_receiver_store_find(&store, key) != NULL);
    assert(mog_dedup_find(&dedup, key)->durable);

    /* Crash after durable RECEIVED but before presentation: reboot must offer
     * exactly the same logical key for presentation and a repair ACK. */
    mog_receiver_record_t reboot_records[8];
    mog_receiver_store_t rebooted;
    recover_store(&rebooted, reboot_records, 8);
    assert(mog_receive_flow_restore(&dedup, &rebooted) == MOG_RECEIVE_FLOW_OK);
    present = ack = false;
    assert(mog_receive_flow_receive(&rebooted, &dedup, key, 200, &present, &ack) ==
           MOG_RECEIVE_FLOW_DUPLICATE);
    assert(present && ack);

    /* UI accepts key idempotently; PRESENTED is durable before RAM suppression. */
    assert(mog_receive_flow_commit_presented(&rebooted, &dedup, key) ==
           MOG_RECEIVE_FLOW_OK);
    assert(mog_dedup_find(&dedup, key)->delivered_to_chat);

    /* Lost ACK + receiver reboot: same message never presents twice, but ACK is
     * deliberately regenerated so sender convergence does not depend on RAM. */
    mog_receiver_record_t reboot2_records[8];
    mog_receiver_store_t rebooted2;
    recover_store(&rebooted2, reboot2_records, 8);
    assert(mog_receive_flow_restore(&dedup, &rebooted2) == MOG_RECEIVE_FLOW_OK);
    present = ack = true;
    assert(mog_receive_flow_receive(&rebooted2, &dedup, key, 300, &present, &ack) ==
           MOG_RECEIVE_FLOW_DUPLICATE);
    assert(!present && ack);

    /* Cross-origin reuse of PacketId remains a different chat identity. */
    const mog_message_key_t other = {.origin = 43, .packet_id = key.packet_id};
    present = ack = false;
    assert(mog_receive_flow_receive(&rebooted2, &dedup, other, 400, &present, &ack) ==
           MOG_RECEIVE_FLOW_OK);
    assert(present && ack);
    assert(mog_receiver_store_find(&rebooted2, other) != NULL);

    unlink(journal);
    unlink(snap_a);
    unlink(snap_b);
    puts("mog_receive_flow tests: PASS");
    return 0;
}
