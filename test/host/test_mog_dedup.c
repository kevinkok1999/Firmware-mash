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

    /* First receipt presents once and requests an E2E ACK. */
    assert(mog_dedup_receive(&d, key(7, 42), 100, &present, &ack) == MOG_DEDUP_OK);
    assert(present && ack);
    assert(mog_dedup_mark_presented(&d, key(7, 42)) == MOG_DEDUP_OK);
    assert(mog_dedup_mark_ack_sent(&d, key(7, 42)) == MOG_DEDUP_OK);

    /* Same logical message over another transport never duplicates chat, but
     * can re-ACK after the sender lost the previous ACK. */
    present = true; ack = false;
    assert(mog_dedup_receive(&d, key(7, 42), 200, &present, &ack) == MOG_DEDUP_DUPLICATE);
    assert(!present && ack);

    /* Origin is part of identity: equal packet ids from different nodes are
     * independent logical messages. */
    assert(mog_dedup_receive(&d, key(8, 42), 201, &present, &ack) == MOG_DEDUP_OK);
    assert(present && ack);

    /* Receiver reboot: restore durable presented truth, then duplicate packet
     * remains suppressed while still regenerating an ACK. */
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
    assert(mog_dedup_receive(&d, key(2, 999), 0, &present, &ack) == MOG_DEDUP_ERR_FULL);
    assert(d.count == MOG_DEDUP_MAX_ENTRIES);

    /* Invalid identity is rejected. */
    assert(mog_dedup_receive(&d, key(0, 1), 0, &present, &ack) == MOG_DEDUP_ERR_ARG);

    puts("mog_dedup: PASS");
    return 0;
}
