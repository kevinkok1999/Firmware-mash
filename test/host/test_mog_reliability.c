#include <assert.h>
#include <stdio.h>
#include "mog_reliability.h"

int main(void)
{
    mog_reliability_t rel;
    mog_message_key_t key = { .origin = 7u, .packet_id = 99u };
    mog_message_key_t key2 = { .origin = 7u, .packet_id = 100u };
    mog_message_key_t due[2];
    size_t n = 0;
    const mog_reliability_entry_t *e;

    mog_reliability_init(&rel);
    assert(mog_reliability_track(&rel, key, 3u, 100u) == MOG_REL_OK);
    assert(mog_reliability_track(&rel, key, 3u, 100u) == MOG_REL_OK);
    assert(rel.count == 1u);
    assert(mog_reliability_note_send(&rel, key, 1000u) == MOG_REL_OK);
    e = mog_reliability_find(&rel, key);
    assert(e != NULL && e->state == MOG_MSG_WAITING_ACK && e->attempts == 1u);

    assert(mog_reliability_note_link_success(&rel, key) == MOG_REL_OK);
    assert(mog_reliability_find(&rel, key)->state == MOG_MSG_WAITING_ACK);
    assert(mog_reliability_due(&rel, 1099u, due, 2u, &n) == MOG_REL_OK && n == 0u);
    assert(mog_reliability_due(&rel, 1100u, due, 2u, &n) == MOG_REL_OK && n == 1u);
    assert(mog_message_key_equal(due[0], key));

    assert(mog_reliability_note_send(&rel, key, 1100u) == MOG_REL_OK);
    assert(mog_reliability_find(&rel, key)->next_retry_ms == 1300u);
    assert(mog_reliability_note_e2e_ack(&rel, key) == MOG_REL_OK);
    assert(mog_reliability_find(&rel, key)->state == MOG_MSG_DELIVERED);
    assert(mog_reliability_note_e2e_ack(&rel, key) == MOG_REL_OK);
    assert(mog_reliability_note_send(&rel, key, 1300u) == MOG_REL_ERR_STATE);

    assert(mog_reliability_track(&rel, key2, 2u, 50u) == MOG_REL_OK);
    assert(mog_reliability_defer_no_route(&rel, key2) == MOG_REL_OK);
    assert(mog_reliability_find(&rel, key2)->state == MOG_MSG_WAITING_ROUTE);
    puts("mog_reliability: PASS");
    return 0;
}
