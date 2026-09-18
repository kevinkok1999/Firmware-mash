#include "mog_events.h"

static size_t slot_of(const mog_event_queue_t *queue, size_t logical_index) {
    return (queue->head + logical_index) % queue->capacity;
}

mog_event_class_t mog_event_class_for_type(mog_event_type_t type) {
    switch (type) {
        case MOG_EVENT_RF_METRICS_CHANGED:
        case MOG_EVENT_ENERGY_SOURCE_CHANGED:
            return MOG_EVENT_CLASS_TELEMETRY;

        case MOG_EVENT_LINK_RX:
        case MOG_EVENT_LINK_TX_RESULT:
        case MOG_EVENT_LINK_RECOVERED:
        case MOG_EVENT_NEIGHBOR_UP:
        case MOG_EVENT_NEIGHBOR_DOWN:
        case MOG_EVENT_ROUTE_DISCOVERED:
        case MOG_EVENT_ROUTE_AVAILABLE:
        case MOG_EVENT_ROUTE_FAILED:
        case MOG_EVENT_TRANSPORT_RECOVERED:
        case MOG_EVENT_IP_BEARER_UP:
        case MOG_EVENT_IP_BEARER_DOWN:
        case MOG_EVENT_GATEWAY_DISCOVERED:
        case MOG_EVENT_GATEWAY_AVAILABLE:
        case MOG_EVENT_GATEWAY_LOST:
        case MOG_EVENT_FEDERATION_SESSION_UP:
        case MOG_EVENT_FEDERATION_SESSION_DOWN:
        case MOG_EVENT_DELIVERY_ACK:
        case MOG_EVENT_DELIVERY_TIMEOUT:
        case MOG_EVENT_STORE_RETRY:
        case MOG_EVENT_ENERGY_STATE_CHANGED:
        case MOG_EVENT_TX_RESERVE_READY:
            return MOG_EVENT_CLASS_CONTROL;
    }

    /* Unknown enum values fail toward the safer class: never silently shed an
     * event we do not understand as low-value telemetry. */
    return MOG_EVENT_CLASS_CONTROL;
}

int mog_event_queue_init(mog_event_queue_t *queue,
                         mog_event_t *storage,
                         size_t capacity) {
    if (!queue || !storage || capacity == 0) {
        return MOG_EVENTS_ERR_ARG;
    }

    queue->storage = storage;
    queue->capacity = capacity;
    queue->count = 0;
    queue->head = 0;
    queue->telemetry_dropped = 0;
    queue->telemetry_evicted_for_control = 0;
    queue->control_overflow = 0;
    queue->resync_required = false;
    return MOG_EVENTS_OK;
}

static int append_event(mog_event_queue_t *queue, const mog_event_t *event) {
    if (queue->count >= queue->capacity) {
        return MOG_EVENTS_ERR_ARG;
    }
    const size_t tail = slot_of(queue, queue->count);
    queue->storage[tail] = *event;
    queue->count++;
    return MOG_EVENTS_OK;
}

static int evict_oldest_telemetry(mog_event_queue_t *queue) {
    size_t victim = queue->count;
    for (size_t i = 0; i < queue->count; ++i) {
        if (mog_event_class_for_type(queue->storage[slot_of(queue, i)].type) ==
            MOG_EVENT_CLASS_TELEMETRY) {
            victim = i;
            break;
        }
    }
    if (victim == queue->count) {
        return -1;
    }

    for (size_t i = victim; i + 1 < queue->count; ++i) {
        queue->storage[slot_of(queue, i)] = queue->storage[slot_of(queue, i + 1)];
    }
    queue->count--;
    queue->telemetry_evicted_for_control++;
    return 0;
}

int mog_event_queue_push(mog_event_queue_t *queue, const mog_event_t *event) {
    if (!queue || !event || !queue->storage || queue->capacity == 0) {
        return MOG_EVENTS_ERR_ARG;
    }

    if (queue->count < queue->capacity) {
        return append_event(queue, event);
    }

    if (mog_event_class_for_type(event->type) == MOG_EVENT_CLASS_TELEMETRY) {
        queue->telemetry_dropped++;
        return MOG_EVENTS_DROPPED_TELEMETRY;
    }

    if (evict_oldest_telemetry(queue) == 0) {
        return append_event(queue, event);
    }

    queue->control_overflow++;
    queue->resync_required = true;
    return MOG_EVENTS_ERR_CONTROL_OVERFLOW;
}

int mog_event_queue_pop(mog_event_queue_t *queue, mog_event_t *out) {
    if (!queue || !out || !queue->storage || queue->capacity == 0) {
        return MOG_EVENTS_ERR_ARG;
    }
    if (queue->count == 0) {
        return MOG_EVENTS_ERR_EMPTY;
    }

    *out = queue->storage[queue->head];
    queue->head = (queue->head + 1) % queue->capacity;
    queue->count--;
    if (queue->count == 0) {
        queue->head = 0;
    }
    return MOG_EVENTS_OK;
}

size_t mog_event_queue_count(const mog_event_queue_t *queue) {
    return queue ? queue->count : 0;
}

void mog_event_queue_clear_resync(mog_event_queue_t *queue) {
    if (queue) {
        queue->resync_required = false;
    }
}
