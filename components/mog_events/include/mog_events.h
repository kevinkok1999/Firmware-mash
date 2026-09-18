#ifndef MOG_EVENTS_H
#define MOG_EVENTS_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "mog_core.h"

#ifdef __cplusplus
extern "C" {
#endif

typedef enum {
    MOG_EVENT_LINK_RX = 0,
    MOG_EVENT_LINK_TX_RESULT,
    MOG_EVENT_LINK_RECOVERED,
    MOG_EVENT_NEIGHBOR_UP,
    MOG_EVENT_NEIGHBOR_DOWN,
    MOG_EVENT_ROUTE_DISCOVERED,
    MOG_EVENT_ROUTE_AVAILABLE,
    MOG_EVENT_ROUTE_FAILED,
    MOG_EVENT_TRANSPORT_RECOVERED,
    MOG_EVENT_IP_BEARER_UP,
    MOG_EVENT_IP_BEARER_DOWN,
    MOG_EVENT_GATEWAY_DISCOVERED,
    MOG_EVENT_GATEWAY_AVAILABLE,
    MOG_EVENT_GATEWAY_LOST,
    MOG_EVENT_FEDERATION_SESSION_UP,
    MOG_EVENT_FEDERATION_SESSION_DOWN,
    MOG_EVENT_DELIVERY_ACK,
    MOG_EVENT_DELIVERY_TIMEOUT,
    MOG_EVENT_STORE_RETRY,
    MOG_EVENT_RF_METRICS_CHANGED,
    MOG_EVENT_ENERGY_STATE_CHANGED,
    MOG_EVENT_ENERGY_SOURCE_CHANGED,
    MOG_EVENT_TX_RESERVE_READY,
} mog_event_type_t;

typedef enum {
    MOG_EVENT_CLASS_TELEMETRY = 0,
    MOG_EVENT_CLASS_CONTROL = 1,
} mog_event_class_t;

typedef struct {
    mog_event_type_t type;
    mog_message_key_t message;
    uint32_t source;
    uint32_t arg0;
    uint32_t arg1;
} mog_event_t;

typedef enum {
    MOG_EVENTS_OK = 0,
    MOG_EVENTS_DROPPED_TELEMETRY = 1,
    MOG_EVENTS_ERR_ARG = -1,
    MOG_EVENTS_ERR_CONTROL_OVERFLOW = -2,
    MOG_EVENTS_ERR_EMPTY = -3,
} mog_events_result_t;

typedef struct {
    mog_event_t *storage;
    size_t capacity;
    size_t count;
    size_t head;
    uint32_t telemetry_dropped;
    uint32_t telemetry_evicted_for_control;
    uint32_t control_overflow;
    bool resync_required;
} mog_event_queue_t;

mog_event_class_t mog_event_class_for_type(mog_event_type_t type);

int mog_event_queue_init(mog_event_queue_t *queue,
                         mog_event_t *storage,
                         size_t capacity);

/*
 * Bounded overflow policy. Priority is derived from event TYPE, never supplied
 * by the caller, so a DELIVERY_ACK cannot accidentally be labelled telemetry.
 *
 * - telemetry on a full queue is dropped and counted;
 * - control on a full queue evicts the oldest telemetry event if one exists;
 * - if a full queue contains only control events, the new control event is not
 *   silently lost: push returns CONTROL_OVERFLOW and sets resync_required.
 */
int mog_event_queue_push(mog_event_queue_t *queue, const mog_event_t *event);
int mog_event_queue_pop(mog_event_queue_t *queue, mog_event_t *out);

size_t mog_event_queue_count(const mog_event_queue_t *queue);
void mog_event_queue_clear_resync(mog_event_queue_t *queue);

#ifdef __cplusplus
}
#endif

#endif /* MOG_EVENTS_H */
