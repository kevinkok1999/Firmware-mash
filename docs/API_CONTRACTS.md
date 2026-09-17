# Internal API Contracts — Pre-Build Draft

These are architecture contracts, not final source headers. Exact names/types may be adapted to the selected foundation after baseline approval, but changing the invariants requires an ADR.

## Common identity

```c
typedef uint32_t mog_node_id_t;
typedef uint64_t mog_packet_id_t;

typedef enum {
    MOG_LINK_LORA = 1,
    MOG_LINK_ESPNOW,
    MOG_LINK_IP,
    MOG_LINK_NAN,
    MOG_LINK_BACKSCATTER,
} mog_link_type_t;
```

A logical packet gets one `mog_packet_id_t` before route selection. Transport-specific sequence/frame IDs never replace it. Conversation identity is separate from transport and remains stable across path changes.

## Transport capability flags

Capabilities are transport facts, not separate routing engines.

Conceptual flags include:

```text
MOG_CAP_DIRECT
MOG_CAP_BROADCAST
MOG_CAP_RSSI
MOG_CAP_SNR
MOG_CAP_ESPNOW_NORMAL
MOG_CAP_ESPNOW_LR
MOG_CAP_IP
MOG_CAP_GATEWAY
MOG_CAP_METERED
MOG_CAP_LOW_POWER
MOG_CAP_DISCOVERY
```

For ESP-NOW, Normal and Long Range are modes/capabilities of the same `MOG_LINK_ESPNOW` adapter. The user does not select them per message.

Conceptual ESP-NOW mode state:

```c
typedef enum {
    MOG_ESPNOW_MODE_AUTO = 0,
    MOG_ESPNOW_MODE_NORMAL,
    MOG_ESPNOW_MODE_LR,
} mog_espnow_mode_t;
```

`AUTO` is the normal policy-facing mode. Actual support for LR must be confirmed against the pinned ESP-IDF/target before enabling it in a stable build.

## IP bearer identity

Wi-Fi and cellular are bearer providers below one logical IP transport.

```c
typedef enum {
    MOG_IP_BEARER_NONE = 0,
    MOG_IP_BEARER_WIFI,
    MOG_IP_BEARER_CELLULAR,
    MOG_IP_BEARER_ETHERNET,
    MOG_IP_BEARER_OTHER,
} mog_ip_bearer_t;
```

The user does not get a separate conversation for each bearer. A bearer change does not create a new PacketId.

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
    uint16_t monetary_cost_class;
    uint8_t congestion;
    uint8_t confidence;
    uint8_t trust_confidence;
} mog_link_metrics_t;
```

Metrics are observations, not direct routing decisions. Metrics that do not apply to a transport are marked unavailable/unknown rather than fabricated. `monetary_cost_class` is a bounded policy signal for metered connectivity, not a billing engine.

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

## IP NetifProvider

`mog_transport_ip` consumes a replaceable IP bearer provider rather than modem/Wi-Fi details.

Conceptual provider operations:

```text
init()
start()
stop()
available()
bearer_type()
local_address_state()
connectivity_state()
metrics()
health()
```

A cellular provider may internally use a maintained modem/PPP implementation. A Wi-Fi provider may use the selected foundation/ESP-IDF station/network stack. Routing code does not issue AT commands or manage Wi-Fi association directly.

Provider failure must emit bounded state/events and degrade only the IP path.

## Gateway record

Gateway state is bounded, expiring evidence owned by GatewayManager, not a second route table.

Conceptual record:

```c
typedef struct {
    mog_node_id_t gateway_id;
    uint32_t capability_flags;
    uint32_t protocol_version;
    uint32_t last_seen_ms;
    uint32_t expires_at_ms;
    uint16_t latency_ms;
    uint16_t loss_permille;
    uint16_t energy_cost;
    uint16_t monetary_cost_class;
    uint8_t trust_confidence;
    uint8_t route_confidence;
    bool authenticated;
    bool reachable;
} mog_gateway_t;
```

Exact identity/authentication representation follows the selected foundation/security design.

## GatewayManager / GatewayDiscovery

GatewayManager provides the equivalent of:

```text
observe_advertisement(advertisement)
mark_session_up(gateway_id, evidence)
mark_session_down(gateway_id, reason)
lookup(gateway_id)
select_candidates(destination, policy)
age(now)
health()
```

GatewayDiscovery may source advertisements from:

```text
mesh/radio announcement
local LAN discovery
configured bootstrap peer
already-authenticated federation peer
```

Hard rules:

- advertisements are bounded, versioned and expire;
- authentication/trust is separate from mere reachability;
- discovery cannot install routes by bypassing HybridRouter;
- no unlimited global gateway table;
- one unavailable bootstrap service cannot erase already-known active peers;
- precise GPS location is not required for discovery.

## Network events

Conceptual event types:

```text
LINK_RX
LINK_TX_RESULT
LINK_RECOVERED
NEIGHBOR_UP
NEIGHBOR_DOWN
ROUTE_DISCOVERED
ROUTE_AVAILABLE
ROUTE_FAILED
TRANSPORT_RECOVERED
IP_BEARER_UP
IP_BEARER_DOWN
GATEWAY_DISCOVERED
GATEWAY_AVAILABLE
GATEWAY_LOST
FEDERATION_SESSION_UP
FEDERATION_SESSION_DOWN
DELIVERY_ACK
DELIVERY_TIMEOUT
STORE_RETRY
RF_METRICS_CHANGED
ENERGY_STATE_CHANGED
ENERGY_SOURCE_CHANGED
TX_RESERVE_READY
```

`LINK_RECOVERED`, `NEIGHBOR_UP`, `ROUTE_DISCOVERED`, `ROUTE_AVAILABLE`, `TRANSPORT_RECOVERED` and `GATEWAY_AVAILABLE` may make a durable `WAITING_ROUTE` message immediately retry-eligible. `TX_RESERVE_READY` may make an energy-deferred attempt eligible for reevaluation when compatible hardware exists. None of these events bypass ReliabilityManager, AirtimeManager or bounded anti-storm backoff/jitter.

All events use a bounded queue/pool. Overflow increments an observable health counter and follows a documented drop/backpressure policy.

## EnergyManager

Energy policy is separate from routing and transport ownership.

Conceptual state:

```c
typedef enum {
    MOG_ENERGY_EXTERNAL_POWER = 0,
    MOG_ENERGY_NORMAL,
    MOG_ENERGY_CONSERVE,
    MOG_ENERGY_CRITICAL,
    MOG_ENERGY_SURVIVAL,
} mog_energy_state_t;
```

Conceptual provider sample:

```c
typedef struct {
    bool available;
    bool external_power;
    uint32_t source_flags;
    uint32_t voltage_mv;
    int32_t current_ua;
    int32_t power_uw;
    uint32_t storage_voltage_mv;
    uint8_t confidence;
    uint32_t sample_age_ms;
} mog_energy_sample_t;
```

Unknown current/power/reservoir values must be representable as unknown; they are never fabricated.

Providers expose the equivalent of:

```text
init()
available()
sample()
capabilities()
health()
```

Consumers receive an immutable normalized `EnergyPolicySnapshot` containing at least energy state, external-power presence, optional harvest availability/power class, relay/discovery/multipath/IP-background budgets and an energy-cost bias.

Hard rules:

- `EnergyManager` is the only device energy-policy authority;
- HybridRouter consumes normalized energy information only;
- no routing module reads PMIC/ADC registers directly;
- an RF-harvest provider is not a `TransportAdapter`;
- harvesting hardware may be absent without breaking boot, messaging or LoRa;
- policy thresholds use hysteresis and are frozen from measured board/hardware evidence rather than guessed constants.

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
    uint8_t gateway_count;
    uint16_t etx_x100;
    uint16_t delivery_probability;
    uint16_t latency_ms;
    uint16_t airtime_ms;
    uint16_t congestion_cost;
    uint16_t energy_cost;
    uint16_t monetary_cost_class;
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

No adapter or gateway manager owns a RouteSet.

## RouteScoreEngine

Input includes ETX/PDR, latency, airtime, congestion, energy, optional metered-data cost, trust confidence, freshness, stability and diversity. Output must include enough diagnostics to explain why a path won. Scoring configuration is versioned (`route-score-v1`, etc.) so simulator comparisons remain reproducible.

Energy cost is bounded input from link metrics plus EnergyPolicySnapshot. Reliability may not be sacrificed for tiny energy savings without explicit policy evidence. IP availability is not a hardcoded winner.

Do not hardcode a permanent score before simulator and hardware data exist.

## PathDiversityEvaluator

Given primary and candidate backup, report shared failure domains such as:

- same first hop;
- shared intermediate nodes;
- shared gateway/federation peer;
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

CRITICAL does not automatically duplicate traffic. A second independent path is permitted only if policy, airtime and energy budgets allow it.

Energy-driven deferral or successful IP socket write must not be represented as Delivered unless ReliabilityManager obtains the required end-to-end evidence.

## Dedup

`seen(PacketId)` is evaluated before application delivery. A duplicate arriving by another path is not shown twice, but may contribute link/path evidence. The destination may re-ACK a duplicate when required to complete sender-side delivery state.

## MessageStore

Conceptual states:

```text
CREATED
READY
SENDING
WAITING_ACK
WAITING_ROUTE
DEFERRED
DELIVERED
EXPIRED
FAILED_PERMANENT
```

Storage is bounded, encrypted at the appropriate security boundary, TTL-controlled and uses deterministic priority/eviction behavior.

Energy policy or loss of Internet may defer work but does not mutate the logical PacketId or discard a committed message outside normal TTL/priority/full-store policy.

## AirtimeManager

Every SX1262 transmit request passes one gate. It receives packet priority, estimated airtime, retry/failover context and region configuration, and can defer/reject transmission when budgets would be exceeded. No module may bypass it.

## 2.4 GHz RadioScheduler

Coordinates ESP-NOW, Wi-Fi/NAN, BLE and scans on the ESP32-S3 shared 2.4 GHz radio. Requests provide priority, deadline/duration estimate and preemptibility. It is not a routing engine. EnergyManager may provide a background-radio budget but cannot directly create routes.

Wi-Fi IP use and ESP-NOW coexistence therefore require explicit scheduling/hardware tests rather than assuming simultaneous ideal operation.

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

RF energy harvesting is separate from RfAssistProvider and belongs under EnergyManager.

## NetworkHealthManager

Must expose counters/high-water marks for packet/event pool pressure, route-table pressure, peer exhaustion, retries, loops detected, store pressure, airtime pressure, IP reconnects, gateway/session pressure, rejected/expired gateway advertisements, energy-policy deferrals, invalid energy samples, optional harvester health and stalled adapter/task state.

## Time contract

Routing/retry/store/energy/gateway timers use one monotonic abstraction. Elapsed/deadline helpers must be wrap-safe. State is represented by explicit enums/flags rather than overloading timestamp sentinel values where avoidable.
