# Firmware-mash Implementation Blueprint

## Purpose

Turn the approved architecture into one code-ready implementation sequence. Exact foundation hooks, capacities, partition sizes and hardware thresholds are frozen only from real baseline/hardware evidence; ownership and dependency direction are fixed unless superseded by ADR.

## Coding order after baseline approval

### 1. Foundation integration seam

Reproduce/import the selected T-Deck Plus foundation at the pinned commit without behavior changes. Preserve board/build/test coherence and provenance.

### 2. Standalone internal persistence

Create/adapt `components/mog_message_store/` with a backend-independent durable internal MessageStore. Requirements: no microSD dependency, power-loss recovery, deterministic TTL/priority/full-store policy, bounded RAM index, isolated identity/config domain.

### 3. Core packet/event primitives

Create/adapt:

```text
components/mog_packet/
components/mog_events/
components/mog_core/
```

Freeze one logical PacketId before route selection. Conversation identity belongs to contact/identity state, never transport. All packet/event pools are bounded.

### 4. EnergyManager foundation

Create/adapt `components/mog_energy/`. Integrate reliable battery/USB/external-power telemetry, hysteretic states and immutable `EnergyPolicySnapshot`. No harvesting hardware required. Unknown measurements remain unknown; no high-frequency telemetry is persisted.

### 5. LoRa transport + airtime gate

Create/adapt:

```text
components/mog_transport_lora/
components/mog_airtime/
```

Wrap the proven SX1262 path without changing baseline behavior. Every LoRa TX passes AirtimeManager.

### 6. Neighbor + HybridRouter seam

Create/adapt:

```text
components/mog_neighbor/
components/mog_routing/
components/mog_route_discovery/
```

One network-state writer/task only. First target is behavior-equivalent LoRa-only routing. HybridRouter consumes normalized energy information, never raw PMIC state.

### 7. Reliability + dedup + sender delayed delivery

Create/adapt:

```text
components/mog_reliability/
components/mog_dedup/
```

Own E2E ACK correlation, bounded retries/backoff, delivery timeout, dedup and MessageStore handoff. Link/socket/custody success never equals Delivered. `WAITING_ROUTE` survives reboot and becomes retry-eligible from credible connectivity events.

### 8. Multipath/failover + route scoring

Create/adapt:

```text
components/mog_multipath/
components/mog_route_score/
components/mog_path_diversity/
```

Maintain a small bounded candidate set. Cached alternate before broad rediscovery. Preserve one PacketId across failover. Route scoring uses measured reliability/latency/airtime/congestion/energy/freshness/diversity with hysteresis.

### 9. Optional custody/store-carry-forward core

Create/adapt:

```text
components/mog_custody/
  include/mog_custody.h
  mog_custody.c/.cpp
  mog_custody_policy.c/.cpp
```

Integrate with ReliabilityManager + MessageStore according to `STORE_CARRY_CUSTODY_CONTRACT.md` and ADR 0008.

Initial proof is LoRa-only so durability/ownership semantics are isolated from optional transport complexity.

Requirements:

- durable relay commit precedes custody acceptance;
- same PacketId/protected envelope/chat;
- acceptance never equals Delivered;
- bounded single-active-holder-oriented ownership/reconciliation;
- deterministic TTL/storage/energy rejection;
- relay reboot recovery;
- custody OFF preserves ordinary sender WAITING_ROUTE.

### 10. ESP-NOW Normal/LR adapter

Create/adapt `components/mog_transport_espnow/` plus `mog_radio_scheduler`. Normal/LR are modes/capabilities of one adapter. HybridRouter uses measured link evidence. No normal-user transport selection.

### 11. IP backhaul + gateway federation

Create/adapt:

```text
components/mog_transport_ip/
components/mog_netif_wifi/
components/mog_netif_cellular/
components/mog_gateway/
components/mog_gateway_discovery/
```

Order:

1. NetifProvider interface + host/mock;
2. stock T-Deck Wi-Fi provider;
3. one outbound authenticated IP/federation session;
4. bounded GatewayManager + authenticated/expiring advertisements;
5. feed gateway/IP reachability evidence into HybridRouter;
6. prove LoRa -> gateway -> IP -> gateway -> LoRa with same PacketId/chat;
7. prove IP loss fallback/WAITING_ROUTE;
8. prove IP recovery automatic queued retry;
9. add multiple bootstrap/federation peers and failover tests;
10. add cellular provider only against selected real modem/PPP hardware.

Wi-Fi/cellular are bearers of one IP transport, not routers/chats. No single mandatory cloud server owns conversation state.

### 12. Cross-transport recovery integration

Normalize recovery events such as:

```text
NEIGHBOR_UP
LINK_RECOVERED
ROUTE_AVAILABLE
TRANSPORT_RECOVERED
GATEWAY_AVAILABLE
FEDERATION_SESSION_UP
CUSTODY_FORWARD_ELIGIBLE
```

Queued/custody-held messages are reevaluated with bounded anti-storm policy. No distance trigger.

### 13. Health + metrics + controller pass

Create/adapt:

```text
components/mog_health/
components/mog_metrics/
components/mog_rf_intelligence/
```

Expose bounded-resource pressure, retries, route churn, storage health, custody offers/accepts/rejects, gateway/session pressure, IP reconnects, airtime, memory high-water and energy state metrics. Perform a red-team controller review before UI/release integration proceeds.

### 14. Smartphone-like UI integration

Normal visible states include Sending, Waiting for connection, Queued, Delivered, Weak link, Searching, Mesh only, Internet assist available, Storage warning, Energy saving and Recovery required.

One contact is always one conversation. Internal route/custody/gateway transitions do not create extra chats or duplicate bubbles. Optional user network policy is only high-level (`AUTO`, `OFF-GRID ONLY`, `INTERNET ASSIST`).

### 15. Ambient RF Energy Assist software seam

Create `components/mog_energy_rf_harvest/` with unavailable/mock provider first. Real hardware provider remains LAB until compatible rectenna/PMIC/storage hardware is measured. `TX_RESERVE_READY` never bypasses ReliabilityManager/AirtimeManager.

### 16. Optional lab transports/hardware providers

Only after stable-core evidence: NAN, RF assist, backscatter, advanced NAT traversal and target-specific experimental hardware. All remain removable.

### 17. Release-engineering handoff

When the selected tier passes evidence gates, hand off to `FLASHER_RELEASE_CONTRACT.md`:

- clean pinned build;
- required configs/tests;
- generated partition/flash metadata;
- merged/supported image set;
- manifest + SHA-256;
- one-click/web-flasher descriptor where supported;
- exact recovery path;
- reject STABLE if hardware evidence is missing.

## Missing-code implementation rule

If required project code does not exist upstream, implement original `mog_` integration/policy/provider code rather than silently dropping the capability. This includes MessageStore shims, EnergyManager providers, custody state machine, ESP-NOW glue, IP NetifProviders, GatewayManager/Discovery, federation/session framing, simulator models, UI bindings and metrics.

Rules:

- use documented SDK/vendor/datasheet interfaces;
- prefer maintained vendor drivers over unnecessary forks;
- preserve provenance/licenses for reused code;
- reject incompatible-license copying;
- test new state machines/policy code;
- optional hardware code exposes safe unavailable behavior;
- never invent custom cryptographic primitives.

## Dependency direction

```text
UI/Application
  -> Reliability / Message service
       -> optional Custody policy/store ownership
  -> HybridRouter <---- EnergyPolicySnapshot
  -> Transport interfaces
  -> Radio/netif/board drivers

GatewayDiscovery -> GatewayManager -> HybridRouter evidence
Wi-Fi/cellular -> NetifProvider -> IPTransport
MessageStore <-> Reliability/Custody durable commands
Board/PMIC telemetry -> EnergyManager
Optional RF harvest HW -> EnergyManager
```

Custody does not choose routes. GatewayManager does not own routes. MessageStore does not discover routes. Transport adapters do not own delivery truth.

## Concurrency model

- `network_task`: sole writer of routing/neighbor/reliability/custody networking state;
- radio/netif callbacks: bounded events only;
- storage worker: bounded durable I/O;
- IP/federation worker: bounded session I/O, event publication only;
- gateway discovery worker/timer: bounded advertisements/expiry;
- energy worker/timer: bounded sampling/snapshots;
- UI task: snapshot/event consumer only.

No hidden routing/delivery engine in an adapter/provider.

## Memory policy

All target-side packet/event/ACK/route/peer/gateway/custody/session/work structures are fixed/bounded. Persist only recovery-critical state. Exact capacities are frozen after measured baseline resource evidence.

## Definition of implementation-ready

Coding proceeds only after valid baseline approval and final controller review. No coding task should need to reopen routing ownership, PacketId/chat identity, delivery truth, no-SD policy, EnergyManager ownership, gateway/IP hierarchy, custody semantics or release-tier evidence rules.
