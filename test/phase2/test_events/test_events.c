#include "mog_events.h"

#include <assert.h>
#include <stdio.h>

static mog_event_t telemetry(uint32_t n) {
    return (mog_event_t){
        .type = MOG_EVENT_RF_METRICS_CHANGED,
        .arg0 = n,
    };
}

static mog_event_t control(mog_event_type_t type, uint32_t n) {
    return (mog_event_t){
        .type = type,
        .arg0 = n,
    };
}

static void test_type_owns_priority(void) {
    assert(mog_event_class_for_type(MOG_EVENT_DELIVERY_ACK) == MOG_EVENT_CLASS_CONTROL);
    assert(mog_event_class_for_type(MOG_EVENT_ROUTE_AVAILABLE) == MOG_EVENT_CLASS_CONTROL);
    assert(mog_event_class_for_type(MOG_EVENT_RF_METRICS_CHANGED) == MOG_EVENT_CLASS_TELEMETRY);
    assert(mog_event_class_for_type(MOG_EVENT_ENERGY_SOURCE_CHANGED) == MOG_EVENT_CLASS_TELEMETRY);
    assert(mog_event_class_for_type((mog_event_type_t)999) == MOG_EVENT_CLASS_CONTROL);
}

static void test_control_evicts_oldest_telemetry(void) {
    mog_event_t storage[3];
    mog_event_queue_t q;
    assert(mog_event_queue_init(&q, storage, 3) == MOG_EVENTS_OK);

    mog_event_t a = telemetry(1);
    mog_event_t b = control(MOG_EVENT_ROUTE_AVAILABLE, 2);
    mog_event_t c = telemetry(3);
    assert(mog_event_queue_push(&q, &a) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &b) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &c) == MOG_EVENTS_OK);

    mog_event_t ack = control(MOG_EVENT_DELIVERY_ACK, 4);
    assert(mog_event_queue_push(&q, &ack) == MOG_EVENTS_OK);
    assert(q.telemetry_evicted_for_control == 1);
    assert(q.telemetry_dropped == 0);
    assert(q.control_overflow == 0);
    assert(!q.resync_required);
    assert(mog_event_queue_count(&q) == 3);

    mog_event_t out;
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK);
    assert(out.type == MOG_EVENT_ROUTE_AVAILABLE);
    assert(out.arg0 == 2);
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK);
    assert(out.type == MOG_EVENT_RF_METRICS_CHANGED);
    assert(out.arg0 == 3);
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK);
    assert(out.type == MOG_EVENT_DELIVERY_ACK);
    assert(out.arg0 == 4);
}

static void test_telemetry_drops_when_full(void) {
    mog_event_t storage[2];
    mog_event_queue_t q;
    assert(mog_event_queue_init(&q, storage, 2) == MOG_EVENTS_OK);

    mog_event_t a = control(MOG_EVENT_ROUTE_AVAILABLE, 1);
    mog_event_t b = control(MOG_EVENT_DELIVERY_ACK, 2);
    mog_event_t t = telemetry(3);
    assert(mog_event_queue_push(&q, &a) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &b) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &t) == MOG_EVENTS_DROPPED_TELEMETRY);
    assert(q.telemetry_dropped == 1);
    assert(mog_event_queue_count(&q) == 2);
}

static void test_control_overflow_requires_resync(void) {
    mog_event_t storage[2];
    mog_event_queue_t q;
    assert(mog_event_queue_init(&q, storage, 2) == MOG_EVENTS_OK);

    mog_event_t a = control(MOG_EVENT_ROUTE_AVAILABLE, 1);
    mog_event_t b = control(MOG_EVENT_DELIVERY_ACK, 2);
    mog_event_t c = control(MOG_EVENT_DELIVERY_TIMEOUT, 3);
    assert(mog_event_queue_push(&q, &a) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &b) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &c) == MOG_EVENTS_ERR_CONTROL_OVERFLOW);
    assert(q.control_overflow == 1);
    assert(q.resync_required);
    assert(mog_event_queue_count(&q) == 2);

    mog_event_queue_clear_resync(&q);
    assert(!q.resync_required);
}

static void test_ring_wrap_keeps_fifo_order(void) {
    mog_event_t storage[3];
    mog_event_queue_t q;
    assert(mog_event_queue_init(&q, storage, 3) == MOG_EVENTS_OK);

    mog_event_t a = control(MOG_EVENT_NEIGHBOR_UP, 1);
    mog_event_t b = control(MOG_EVENT_ROUTE_AVAILABLE, 2);
    mog_event_t c = control(MOG_EVENT_DELIVERY_ACK, 3);
    mog_event_t d = control(MOG_EVENT_STORE_RETRY, 4);
    assert(mog_event_queue_push(&q, &a) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &b) == MOG_EVENTS_OK);

    mog_event_t out;
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK);
    assert(out.arg0 == 1);

    assert(mog_event_queue_push(&q, &c) == MOG_EVENTS_OK);
    assert(mog_event_queue_push(&q, &d) == MOG_EVENTS_OK);

    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK && out.arg0 == 2);
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK && out.arg0 == 3);
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_OK && out.arg0 == 4);
    assert(mog_event_queue_pop(&q, &out) == MOG_EVENTS_ERR_EMPTY);
}

int main(void) {
    test_type_owns_priority();
    test_control_evicts_oldest_telemetry();
    test_telemetry_drops_when_full();
    test_control_overflow_requires_resync();
    test_ring_wrap_keeps_fifo_order();
    puts("test_events: PASS");
    return 0;
}
