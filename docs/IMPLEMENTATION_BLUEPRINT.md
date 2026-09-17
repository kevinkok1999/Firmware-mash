# Firmware-mash Implementation Blueprint

## Purpose

This document turns the approved architecture into a code-ready implementation sequence. Exact foundation hook names and final resource capacities may only be frozen after the real T-Deck Plus baseline is reproduced, but ownership, behavior and dependency direction are fixed unless an ADR supersedes them.

## Coding order after baseline approval

### 1. Foundation integration seam

Reproduce/import the selected T-Deck Plus foundation at the pinned commit without behavior changes. Preserve upstream board/build coherence.

### 2. Standalone internal persistence

Create/adapt:

```text
components/mog_message_store/
  CMakeLists.txt
  include/mog_message_store.h
  mog_message_store.c/.cpp
  mog_message_store_backend.c/.cpp
  mog_message_store_recovery.c/.cpp
```

Responsibilities: durable pending-message queue in internal flash, no microSD dependency, power-loss recovery, deterministic TTL/priority/full-store policy, bounded RAM index rebuilt at boot. Identity/config remain isolated from high-churn queue storage.

### 3. Core packet/event primitives

```text
components/mog_packet/
components/mog_events/
components/mog_core/
```

Freeze one logical PacketId before route selection. Use bounded packet/event pools. Radio callbacks enqueue events and return; they do not mutate route state.

### 4. LoRa transport wrapper + airtime gate

```text
components/mog_transport_lora/
components/mog_airtime/
```

Wrap the foundation SX1262 path without changing baseline behavior. Every LoRa TX passes AirtimeManager.

### 5. Neighbor + HybridRouter seam

```text
components/mog_neighbor/
components/mog_routing/
components/mog_route_discovery/
```

Use one routing-state writer/task. First target is behavior-equivalent LoRa-only forwarding.

### 6. Reliability + dedup

```text
components/mog_reliability/
components/mog_dedup/
```

Own end-to-end ACK correlation, bounded retries, jitter/backoff, delivery timeout, duplicate collapse and MessageStore handoff.

### 7. Multipath/failover

```text
components/mog_multipath/
components/mog_route_score/
components/mog_path_diversity/
```

Maintain a small bounded candidate set. Attempt a fresh cached alternate before broad rediscovery. Preserve one PacketId across failover.

### 8. ESP-NOW transport including Long Range

```text
components/mog_transport_espnow/
  CMakeLists.txt
  include/mog_transport_espnow.h
  mog_transport_espnow.c/.cpp
  mog_espnow_peer_cache.c/.cpp
  mog_espnow_link_mode.c/.cpp

components/mog_radio_scheduler/
```

Normal ESP-NOW and ESP-NOW Long Range are operating modes of one adapter, not independent routing stacks. HybridRouter sees measured link capabilities/quality. The adapter may select NORMAL or LR per compatible peer/link policy. Normal users do not manually choose transport or route.

### 9. Event-driven delayed delivery

A durable message in WAITING_ROUTE becomes immediately eligible for bounded retry when credible connectivity returns, including:

```text
NEIGHBOR_UP
LINK_RECOVERED
ROUTE_DISCOVERED
ROUTE_AVAILABLE
TRANSPORT_RECOVERED
```

There is no fixed 200 m / 500 m / 1 km trigger. The trigger is usable connectivity evidence. Anti-storm jitter/backoff and airtime policy may delay by a small bounded amount, but the user never needs to press Send again.

### 10. Health + metrics

```text
components/mog_health/
components/mog_metrics/
components/mog_rf_intelligence/
```

Expose queue pressure, route churn, retries, link recovery, storage health, airtime pressure, peer pressure and memory high-water marks.

### 11. UI integration

Required user-visible states:

```text
Sending
Waiting for route
Queued
Delivered
Weak link
Searching for route
Storage warning
Radio degraded
```

The UI never requires normal users to choose LoRa, ESP-NOW Normal, ESP-NOW LR, next hop, retry count or route number.

### 12. Optional/lab transports

Only after stable-core evidence:

```text
mog_transport_nan
mog_rf_assist
future external backscatter adapter
```

These remain removable without breaking LoRa-only operation.

## Dependency direction

```text
UI/Application
  -> Reliability / Message service
  -> HybridRouter
  -> Transport interfaces
  -> Radio/board drivers
```

MessageStore is below reliability and does not make routing decisions. Transport adapters report link evidence and transmit frames; they never own route tables.

## Concurrency model

- `network_task`: sole writer of routing/neighbor/reliability state;
- radio callbacks: enqueue small bounded events and return;
- storage worker: bounded durable I/O requested through explicit commands/events;
- UI task: reads snapshots/events, never mutates routing internals;
- no hidden second routing task inside an adapter.

## Memory policy

- fixed/bounded pools for packets, events, ACKs, peers, routes and queued work;
- avoid unbounded target-side container growth;
- avoid per-packet heap allocation where practical;
- keep transient routes/metrics in RAM/PSRAM;
- persist only recovery-critical state.

Exact capacities are measured and frozen after baseline RAM/PSRAM/flash evidence.

## Definition of implementation-ready

Coding may proceed once the baseline gate is PASS and these contracts remain internally consistent. No coding task should need to reopen ownership, transport hierarchy, retry trigger semantics, no-SD policy, PacketId semantics or stable-vs-lab boundaries.