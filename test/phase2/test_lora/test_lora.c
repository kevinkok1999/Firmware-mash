#include "mog_airtime.h"

#include <assert.h>
#include <stdio.h>

typedef struct {
    int send_result;
    bool can_send_result;
    uint32_t cost_ms;
    unsigned send_calls;
    size_t last_len;
    mog_airtime_kind_t last_kind;
} fake_backend_t;

static int fake_send(void *ctx, const uint8_t *data, size_t len, mog_airtime_kind_t kind) {
    fake_backend_t *fake = (fake_backend_t *)ctx;
    assert(data != NULL);
    fake->send_calls++;
    fake->last_len = len;
    fake->last_kind = kind;
    return fake->send_result;
}

static bool fake_can_send(void *ctx, size_t wire_len, mog_airtime_kind_t kind) {
    fake_backend_t *fake = (fake_backend_t *)ctx;
    fake->last_len = wire_len;
    fake->last_kind = kind;
    return fake->can_send_result;
}

static uint32_t fake_cost(void *ctx, size_t wire_len) {
    fake_backend_t *fake = (fake_backend_t *)ctx;
    fake->last_len = wire_len;
    return fake->cost_ms;
}

static mog_airtime_manager_t manager(fake_backend_t *fake, size_t max_wire_len) {
    mog_airtime_manager_t m;
    const mog_airtime_backend_ops_t ops = {
        .send = fake_send,
        .can_send = fake_can_send,
        .cost_ms = fake_cost,
    };
    assert(mog_airtime_manager_init(&m, &ops, fake, max_wire_len) == MOG_AIRTIME_OK);
    return m;
}

static void test_success_and_diagnostics(void) {
    fake_backend_t fake = {
        .send_result = MOG_AIRTIME_BACKEND_OK,
        .can_send_result = true,
        .cost_ms = 123,
    };
    mog_airtime_manager_t m = manager(&fake, 200);
    const uint8_t data[] = {1, 2, 3};

    assert(mog_airtime_can_send(&m, sizeof(data), MOG_AIRTIME_DATA));
    assert(mog_airtime_cost_ms(&m, sizeof(data)) == 123);
    assert(mog_airtime_send(&m, data, sizeof(data), MOG_AIRTIME_DATA) == MOG_AIRTIME_OK);
    assert(fake.send_calls == 1);
    assert(fake.last_kind == MOG_AIRTIME_DATA);
    assert(m.tx_ok == 1);
}

static void test_backend_failures_are_not_relabelled_success(void) {
    fake_backend_t fake = {.can_send_result = true};
    mog_airtime_manager_t m = manager(&fake, 200);
    const uint8_t data[] = {9};

    fake.send_result = MOG_AIRTIME_BACKEND_DENIED;
    assert(mog_airtime_send(&m, data, 1, MOG_AIRTIME_DATA_RETRY) == MOG_AIRTIME_ERR_DENIED);
    assert(m.tx_denied == 1);

    fake.send_result = MOG_AIRTIME_BACKEND_BUSY;
    assert(mog_airtime_send(&m, data, 1, MOG_AIRTIME_ACK) == MOG_AIRTIME_ERR_BUSY);
    assert(m.tx_busy == 1);

    fake.send_result = MOG_AIRTIME_BACKEND_RADIO_ERROR;
    assert(mog_airtime_send(&m, data, 1, MOG_AIRTIME_FORWARD) == MOG_AIRTIME_ERR_RADIO);
    assert(m.tx_radio_error == 1);
}

static void test_oversize_never_reaches_backend(void) {
    fake_backend_t fake = {
        .send_result = MOG_AIRTIME_BACKEND_OK,
        .can_send_result = true,
    };
    mog_airtime_manager_t m = manager(&fake, 10);
    uint8_t data[11] = {0};

    assert(mog_airtime_send(&m, data, sizeof(data), MOG_AIRTIME_DATA) ==
           MOG_AIRTIME_ERR_TOO_LARGE);
    assert(fake.send_calls == 0);
    assert(!mog_airtime_can_send(&m, sizeof(data), MOG_AIRTIME_DATA));
}

int main(void) {
    test_success_and_diagnostics();
    test_backend_failures_are_not_relabelled_success();
    test_oversize_never_reaches_backend();
    puts("test_lora: PASS");
    return 0;
}
