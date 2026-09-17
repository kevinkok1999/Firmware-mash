#include "mog_core.h"

#include <assert.h>
#include <stdint.h>
#include <stdio.h>

typedef struct {
    uint64_t durable_ceiling;
    int fail_read;
    int fail_write;
    unsigned writes;
} mock_store_t;

static int mock_read(uint64_t *ceiling_out, void *ctx) {
    mock_store_t *store = (mock_store_t *)ctx;
    if (!store || !ceiling_out || store->fail_read) {
        return -1;
    }
    *ceiling_out = store->durable_ceiling;
    return 0;
}

static int mock_write(uint64_t ceiling, void *ctx) {
    mock_store_t *store = (mock_store_t *)ctx;
    if (!store || store->fail_write) {
        return -1;
    }
    store->durable_ceiling = ceiling;
    store->writes++;
    return 0;
}

static void test_packet_ids_survive_reboot_without_reuse(void) {
    mock_store_t store = {0};
    mog_packet_id_generator_t first;
    assert(mog_packet_id_generator_init(&first, 8, mock_read, mock_write, &store) == MOG_CORE_OK);
    assert(store.durable_ceiling == 8);

    mog_packet_id_t a = 0;
    mog_packet_id_t b = 0;
    assert(mog_packet_id_next(&first, &a) == MOG_CORE_OK);
    assert(mog_packet_id_next(&first, &b) == MOG_CORE_OK);
    assert(a == 1);
    assert(b == 2);

    /* Simulated hard reboot: volatile generator state disappears but the
     * durable ceiling survives. The new generator must skip the unused tail. */
    mog_packet_id_generator_t second;
    assert(mog_packet_id_generator_init(&second, 8, mock_read, mock_write, &store) == MOG_CORE_OK);
    assert(store.durable_ceiling == 16);

    mog_packet_id_t after_reboot = 0;
    assert(mog_packet_id_next(&second, &after_reboot) == MOG_CORE_OK);
    assert(after_reboot == 8);
    assert(after_reboot != a && after_reboot != b);
}

static void test_boundary_write_failure_issues_nothing(void) {
    mock_store_t store = {0};
    mog_packet_id_generator_t gen;
    assert(mog_packet_id_generator_init(&gen, 3, mock_read, mock_write, &store) == MOG_CORE_OK);

    mog_packet_id_t id = 0;
    assert(mog_packet_id_next(&gen, &id) == MOG_CORE_OK && id == 1);
    assert(mog_packet_id_next(&gen, &id) == MOG_CORE_OK && id == 2);

    store.fail_write = 1;
    const uint64_t next_before = gen.next;
    const uint64_t ceiling_before = gen.durable_ceiling;
    id = 999;
    assert(mog_packet_id_next(&gen, &id) == MOG_CORE_ERR_STORE);
    assert(id == 999);
    assert(gen.next == next_before);
    assert(gen.durable_ceiling == ceiling_before);

    store.fail_write = 0;
    assert(mog_packet_id_next(&gen, &id) == MOG_CORE_OK);
    assert(id == 3);
}

static void test_init_fails_closed(void) {
    mock_store_t read_fail = {.fail_read = 1};
    mog_packet_id_generator_t gen;
    assert(mog_packet_id_generator_init(&gen, 8, mock_read, mock_write, &read_fail) ==
           MOG_CORE_ERR_STORE);

    mock_store_t write_fail = {.fail_write = 1};
    assert(mog_packet_id_generator_init(&gen, 8, mock_read, mock_write, &write_fail) ==
           MOG_CORE_ERR_STORE);
}

static void test_message_key_is_origin_qualified(void) {
    mog_message_key_t a = {.origin = 0x11111111u, .packet_id = 42};
    mog_message_key_t b = {.origin = 0x22222222u, .packet_id = 42};
    mog_message_key_t c = a;

    assert(mog_message_key_is_valid(a));
    assert(!mog_message_key_equal(a, b));
    assert(mog_message_key_equal(a, c));
    assert(!mog_message_key_is_valid((mog_message_key_t){.origin = 0, .packet_id = 42}));
    assert(!mog_message_key_is_valid((mog_message_key_t){.origin = 1, .packet_id = 0}));
}

static void test_lifecycle_terminal_truth(void) {
    assert(mog_message_state_can_transition(MOG_MSG_CREATED, MOG_MSG_READY));
    assert(mog_message_state_can_transition(MOG_MSG_READY, MOG_MSG_WAITING_ROUTE));
    assert(mog_message_state_can_transition(MOG_MSG_WAITING_ROUTE, MOG_MSG_SENDING));
    assert(mog_message_state_can_transition(MOG_MSG_WAITING_ACK, MOG_MSG_DELIVERED));
    assert(mog_message_state_is_terminal(MOG_MSG_DELIVERED));
    assert(!mog_message_state_can_transition(MOG_MSG_DELIVERED, MOG_MSG_SENDING));
    assert(!mog_message_state_can_transition(MOG_MSG_EXPIRED, MOG_MSG_READY));
    assert(!mog_message_state_can_transition(MOG_MSG_FAILED_PERMANENT, MOG_MSG_READY));
}

static void test_wrap_safe_time_helpers(void) {
    assert(mog_time_elapsed32(15u, 10u) == 5u);
    assert(mog_time_elapsed32(3u, UINT32_MAX - 2u) == 6u);

    const uint32_t deadline = UINT32_MAX - 1u;
    assert(!mog_time_reached32(UINT32_MAX - 2u, deadline));
    assert(mog_time_reached32(UINT32_MAX, deadline));
    assert(mog_time_reached32(2u, deadline));
}

int main(void) {
    test_packet_ids_survive_reboot_without_reuse();
    test_boundary_write_failure_issues_nothing();
    test_init_fails_closed();
    test_message_key_is_origin_qualified();
    test_lifecycle_terminal_truth();
    test_wrap_safe_time_helpers();
    puts("test_core: PASS");
    return 0;
}
