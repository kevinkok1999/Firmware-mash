# Internal API Contracts — Pre-Build Draft

These are architecture contracts, not final source headers. Exact names/types may be adapted to the selected foundation after baseline approval, but changing the invariants requires an ADR.

## Common identity

```c
typedef uint32_t mog_node_id_t;
typedef uint64_t mog_packet_id_t;

typedef enum {
    MOG_LINK_LORA = 1,
    MOG_LINK_ESPNOW,
    MOG_LINK_NAN,
    MOG_LINK_TCP,
    MOG_LINK_BACKSCATTER,
} mog_link_type_t;
```

A logical packet gets one `mog_packet_id_t` before route selection. Transport-specific sequence/frame IDs never replace it.

## Link metrics

```c
typedef struct {
    bool available;
    uint16_t mtu;
    int16_t rssi_dbm;
    int16_t snr_db_x10;
    uint16_t latency_ms;
    uint16_t airtime_ms;
    uint16_t loss_permille;
    uint16_t energy_cost;
    uint8_t congestion;
    uint8_t confidence;
} mog_link_metrics_t;
```

Metrics are observations, not direct routing decisions.

## TransportAdapter

Every data-carrying link exposes the equivalent of:

```c
typedef struct {
    int (*init)(void *ctx);
    int (*start)(void *ctx);
    int (*stop)(void *ctx);
    bool (*available)(const void *ctx);
    uint16_t (*mtu)(const void *ctx);
    int (*send)(void *ctx, mog_node_id_t next_hop,
                const uint8_t *data, size_t len);
    int (*metrics)(void *ctx, mog_node_id_t neighbor,
                   mog_link_metrics_t *out);
    uint32_t (*capabilities)(const void *ctx);
} mog_transport_ops_t;
```

Hard rules:

- adapters cannot directly modify route tables;
- callbacks enqueue bounded events and return quickly;
- adapter success is link evidence, not end-to-end delivery evidence;
- disabling an optional adapter cannot break LoRa-only operation.

## Network events

Conceptual event types:

```text
LINK_RX
LINK_TX_RESULT
NEIGHBOR_UP
NEIGHBOR_DOWN
ROUTE_DISCOVERED
ROUTE_FAILED
DELIVERY_ACK
DELIVERY_TIMEOUT
STORE_RETRY
RF_METRICS_CHANGED
```

All events use a bounded queue/pool. Overflow increments an observable health counter and follows a documented drop/backpressure policy.

## Neighbor record

One node may have multiple physical links.

```c
typedef struct {
    mog_node_id_t node;
    mog_link_type_t link;
    uint32_t last_seen_ms;
    int16_t rssi_ewma;
    int16_t snr_ewma;
    uint16_t loss_ewma;
    uint16_t latency_ewma;
    uint32_t tx_success;
    uint32_t tx_failure;
    uint8_t confidence;
} mog_neighbor_link_t;
```

## Candidate path

```c
typedef struct {
    uint32_t path_id;
    mog_node_id_t destination;
    mog_node_id_t next_hop;
    mog_link_type_t first_link;
    uint8_t hop_count;
    uint16_t etx_x100;
    uint16_t delivery_probability;
    uint16_t latency_ms;
    uint16_t airtime_ms;
    uint16_t congestion_cost;
    uint16_t energy_cost;
    uint16_t freshness;
    uint16_t diversity_score;
    uint16_t confidence;
    uint32_t last_success_ms;
    uint32_t last_failure_ms;
    bool valid;
} mog_path_t;
```

The initial design target is at most three paths per destination, but the actual capacity is fixed only after the baseline memory budget is measured.

## RouteSet

HybridRouter owns all RouteSets. At minimum it must support operations equivalent to:

```text
lookup(destination)
select_primary(destination, delivery_policy)
select_alternate(destination, failed_path)
install_candidate(candidate)
invalidate_path(path_id, reason)
age_routes(now)
start_discovery(destination, bounds)
```

No adapter owns a RouteSet.

## RouteScoreEngine

Input includes ETX/PDR, latency, airtime, congestion, energy, freshness, stability and diversity. Output must include enough diagnostics to explain why a path won. Scoring configuration is versioned (`route-score-v1`, etc.) so simulator comparisons remain reproducible.

Do not hardcode a permanent score before simulator and hardware data exist.

## PathDiversityEvaluator

Given primary and candidate backup, report shared failure domains such as:

- same first hop;
- shared intermediate nodes;
- shared links;
- same constrained physical interface/bottleneck.

Backup selection prefers a useful independent alternative rather than merely the second-highest raw score.

## ReliabilityManager

Owns:

```text
end-to-end ACK correlation
bounded retries
retry backoff+jitter
delivery timeout
failover request
store-forward handoff
delivery receipts where enabled
```

Delivery classes:

```text
BEST_EFFORT
NORMAL
CRITICAL
```

CRITICAL does not automatically duplicate traffic. A second independent path is permitted only if policy and airtime budget allow it.

## Dedup

`seen(PacketId)` is evaluated before application delivery. A duplicate arriving by another path is not shown twice, but may contribute link/path evidence.

## MessageStore

Conceptual states:

```text
READY
SENDING
WAITING_ACK
WAITING_ROUTE
DEFERRED
DELIVERED
EXPIRED
```

Storage is bounded, encrypted at the appropriate security boundary, TTL-controlled and uses deterministic priority/eviction behavior.

## AirtimeManager

Every SX1262 transmit request passes one gate. It receives packet priority, estimated airtime, retry/failover context and region configuration, and can defer/reject transmission when budgets would be exceeded. No module may bypass it.

## 2.4 GHz RadioScheduler

Coordinates ESP-NOW, Wi-Fi/NAN, BLE and scans on the ESP32-S3 shared 2.4 GHz radio. Requests provide priority, deadline/duration estimate and preemptibility. It is not a routing engine.

## RfIntelligence

Provides rolling/EWMA observations for link stability, loss and trend. V1 is deliberately statistical, not an opaque ML model.

## RfAssistProvider

Future RIS/passive-reflector control has an advisory interface equivalent to:

```text
available()
capabilities()
apply_profile(profile_id)
measure_before(target)
measure_after(target)
report_gain(target)
```

It does not become a logical hop merely because it changes RF propagation.

## NetworkHealthManager

Must expose counters/high-water marks for packet/event pool pressure, route-table pressure, peer exhaustion, retries, loops detected, store pressure, airtime pressure and stalled adapter/task state.

## Time contract

Routing/retry/store timers use one monotonic abstraction. Elapsed/deadline helpers must be wrap-safe. State is represented by explicit enums/flags rather than overloading timestamp sentinel values where avoidable.
