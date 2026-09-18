#include <assert.h>
#include <stdbool.h>
#include <stdio.h>

#include "mog_dedup.h"

static mog_message_key_t key(uint32_t origin, uint64_t packet)
{
    mog_message_key_t k = {origin, packet};
    return k;
}

int main(void)
{
    mog_dedup_t d;
    bool present = false, ack = false;
    size_t i;

    mog_dedup_init(&d);

    /* First receipt is staged only: a crash here cannot have produced UI or
     * E2E ACK evidence because MessageStore has not committed receiver truth. */
    present = true; ack = true;
    assert(mog_dedup_receive(&d, key(7, 42), 100, &present, &ack) == MOG_DEDUP_OK);
    assert(!present && !ack);
    assert(mog_dedup_mark_presented(&d, key(7, 42)) == MOG_DEDUP_ERR_NOT_DURABLE);
    assert(mog_dedup_mark_ack_sent(&d, key(7, 42)) == MOG_DEDUP_ERR_NOT_DURABLE);

    /* A duplicate racing persistence remains behind the same barrier. */
    present = true; ack = true;
    assert(mog_dedup_receive(&d, key(7, 42), 101, &present, &ack) == MOG_DEDUP_DUPLICATE);
    assert(!present && !ack);

    /* Once authoritative MessageStore commit succeeds, presentation and ACK
     * become safe in that order. */
    assert(mog_dedup_mark_durable(&d, key(7, 42), &present, &ack) == MOG_DEDUP_OK);
    assert(present && ack);
    assert(mog_dedup_mark_presented(&d, key(7, 42)) == MOG_DEDUP_OK);
    assert(mog_dedup_mark_ack_sent(&d, key(7, 42)) == MOG_DEDUP_OK);

    /* Same logical message over another transport never duplicates chat, but
     * can re-ACK after the sender lost the previous ACK. */
    present = true; ack = false;
    assert(mog_dedup_receive(&d, key(7, 42), 200, &present, &ack) == MOG_DEDUP_DUPLICATE);
    assert(!present && ack);

    /* Origin is part of identity. New origin is independently staged. */
    present = true; ack = true;
    assert(mog_dedup_receive(&d, key(8, 42), 201, &present, &ack) == MOG_DEDUP_OK);
    assert(!present && !ack);
    assert(mog_dedup_mark_durable(&d, key(8, 42), &present, &ack) == MOG_DEDUP_OK);
    assert(present && ack);

    /* Receiver reboot after persistence but before presentation: restore says
     * the record is durable and still needs presentation + ACK exactly once. */
    mog_dedup_init(&d);
    assert(mog_dedup_restore(&d, key(9, 77), false, true, 250) == MOG_DEDUP_OK);
    present = false; ack = false;
    assert(mog_dedup_mark_durable(&d, key(9, 77), &present, &ack) == MOG_DEDUP_OK);
    assert(present && ack);
    assert(mog_dedup_mark_presented(&d, key(9, 77)) == MOG_DEDUP_OK);

    /* Receiver reboot after presentation: duplicate remains suppressed while
     * still regenerating an ACK if the sender lost the previous one. */
    mog_dedup_init(&d);
    assert(mog_dedup_restore(&d, key(7, 42), true, false, 100) == MOG_DEDUP_OK);
    present = true; ack = false;
    assert(mog_dedup_receive(&d, key(7, 42), 300, &present, &ack) == MOG_DEDUP_DUPLICATE);
    assert(!present && ack);

    /* Bounded memory fails closed; live dedup truth is never silently evicted. */
    mog_dedup_init(&d);
    for (i = 0; i < MOG_DEDUP_MAX_ENTRIES; ++i) {
        assert(mog_dedup_restore(&d, key(1, (uint64_t)i + 1u), true, false, 0) == MOG_DEDUP_OK);
    }
    present = true; ack = true;
    assert(mog_dedup_receive(&d, key(2, 999), 0, &present, &ack) == MOG_DEDUP_ERR_FULL);
    assert(d.count == MOG_DEDUP_MAX_ENTRIES);

    /* Invalid identity is rejected. */
    assert(mog_dedup_receive(&d, key(0, 1), 0, &present, &ack) == MOG_DEDUP_ERR_ARG);

    puts("mog_dedup: PASS");
    return 0;
}
