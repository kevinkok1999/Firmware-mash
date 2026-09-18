#include "mog_conversation.h"

#include <assert.h>
#include <stdio.h>

static mog_conversation_message_meta_t meta(mog_conversation_id_t conversation,
                                            mog_node_id_t origin,
                                            mog_packet_id_t packet_id,
                                            mog_conversation_seq_t sender_seq) {
    return (mog_conversation_message_meta_t){
        .conversation_id = conversation,
        .message = {.origin = origin, .packet_id = packet_id},
        .sender_seq = sender_seq,
    };
}

static void test_same_chat_identity_is_transport_independent(void) {
    assert(mog_conversation_same_chat(77, 77));
    assert(!mog_conversation_same_chat(77, 78));
    assert(!mog_conversation_same_chat(0, 0));
}

static void test_out_of_order_arrival_flushes_in_sender_order(void) {
    mog_conversation_pending_t pending[4];
    mog_conversation_order_t state;
    assert(mog_conversation_order_init(&state, 77, 0x11111111u, pending, 4, 8,
                                       true, 10) == MOG_CONVERSATION_OK);

    mog_message_key_t ready[5];
    size_t count = 0;

    const mog_conversation_message_meta_t seq12 = meta(77, 0x11111111u, 1200, 12);
    assert(mog_conversation_accept(&state, &seq12, ready, 5, &count) ==
           MOG_CONVERSATION_BUFFERED);
    assert(count == 0);

    const mog_conversation_message_meta_t seq11 = meta(77, 0x11111111u, 1100, 11);
    assert(mog_conversation_accept(&state, &seq11, ready, 5, &count) == MOG_CONVERSATION_OK);
    assert(count == 2);
    assert(ready[0].packet_id == 1100);
    assert(ready[1].packet_id == 1200);
    assert(state.last_presented_seq == 12);
    assert(state.pending_count == 0);
}

static void test_duplicate_and_sequence_conflict_are_distinct(void) {
    mog_conversation_pending_t pending[3];
    mog_conversation_order_t state;
    assert(mog_conversation_order_init(&state, 9, 42, pending, 3, 6,
                                       true, 5) == MOG_CONVERSATION_OK);

    mog_message_key_t ready[4];
    size_t count = 0;

    const mog_conversation_message_meta_t a = meta(9, 42, 100, 7);
    assert(mog_conversation_accept(&state, &a, ready, 4, &count) ==
           MOG_CONVERSATION_BUFFERED);
    assert(mog_conversation_accept(&state, &a, ready, 4, &count) ==
           MOG_CONVERSATION_DUPLICATE);

    const mog_conversation_message_meta_t conflict = meta(9, 42, 101, 7);
    assert(mog_conversation_accept(&state, &conflict, ready, 4, &count) ==
           MOG_CONVERSATION_ERR_SEQUENCE_CONFLICT);
}

static void test_gap_advance_is_explicit_and_never_discards_buffered_data(void) {
    mog_conversation_pending_t pending[4];
    mog_conversation_order_t state;
    assert(mog_conversation_order_init(&state, 15, 88, pending, 4, 10,
                                       true, 20) == MOG_CONVERSATION_OK);

    mog_message_key_t ready[5];
    size_t count = 0;

    const mog_conversation_message_meta_t seq22 = meta(15, 88, 2200, 22);
    assert(mog_conversation_accept(&state, &seq22, ready, 5, &count) ==
           MOG_CONVERSATION_BUFFERED);

    assert(mog_conversation_advance_gap(&state, 21, ready, 5, &count) == MOG_CONVERSATION_OK);
    assert(count == 1);
    assert(ready[0].packet_id == 2200);
    assert(state.last_presented_seq == 22);

    const mog_conversation_message_meta_t seq24 = meta(15, 88, 2400, 24);
    assert(mog_conversation_accept(&state, &seq24, ready, 5, &count) ==
           MOG_CONVERSATION_BUFFERED);
    assert(mog_conversation_advance_gap(&state, 24, ready, 5, &count) ==
           MOG_CONVERSATION_ERR_GAP_CONFLICT);
}

static void test_first_message_can_start_after_reboot_reserve_gap(void) {
    mog_conversation_pending_t pending[2];
    mog_conversation_order_t state;
    assert(mog_conversation_order_init(&state, 123, 7, pending, 2, 8,
                                       false, 0) == MOG_CONVERSATION_OK);

    mog_message_key_t ready[3];
    size_t count = 0;
    const mog_conversation_message_meta_t first_seen = meta(123, 7, 9001, 500);
    assert(mog_conversation_accept(&state, &first_seen, ready, 3, &count) == MOG_CONVERSATION_OK);
    assert(count == 1);
    assert(ready[0].packet_id == 9001);
    assert(state.last_presented_seq == 500);
}

static void test_window_and_identity_fail_closed(void) {
    mog_conversation_pending_t pending[2];
    mog_conversation_order_t state;
    assert(mog_conversation_order_init(&state, 44, 99, pending, 2, 3,
                                       true, 10) == MOG_CONVERSATION_OK);

    mog_message_key_t ready[3];
    size_t count = 0;

    const mog_conversation_message_meta_t too_far = meta(44, 99, 1, 14);
    assert(mog_conversation_accept(&state, &too_far, ready, 3, &count) ==
           MOG_CONVERSATION_ERR_WINDOW);

    const mog_conversation_message_meta_t wrong_chat = meta(45, 99, 2, 11);
    assert(mog_conversation_accept(&state, &wrong_chat, ready, 3, &count) ==
           MOG_CONVERSATION_ERR_ARG);

    const mog_conversation_message_meta_t wrong_origin = meta(44, 100, 3, 11);
    assert(mog_conversation_accept(&state, &wrong_origin, ready, 3, &count) ==
           MOG_CONVERSATION_ERR_ARG);
}

int main(void) {
    test_same_chat_identity_is_transport_independent();
    test_out_of_order_arrival_flushes_in_sender_order();
    test_duplicate_and_sequence_conflict_are_distinct();
    test_gap_advance_is_explicit_and_never_discards_buffered_data();
    test_first_message_can_start_after_reboot_reserve_gap();
    test_window_and_identity_fail_closed();
    puts("test_conversation: PASS");
    return 0;
}
