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

### 4. EnergyManager foundation

Create/adapt:

```text
components/mog_energy/
  CMakeLists.txt
  include/mog_energy.h
  mog_energy.c/.cpp
  mog_energy_policy.c/.cpp
  mog_energy_metrics.c/.cpp
```

Integrate reliable battery/USB/external-power telemetry exposed by the selected foundation behind one abstraction. Implement explicit hysteretic energy states and immutable `EnergyPolicySnapshot` output.

At this stage:

- no real RF-harvesting hardware is required;
- RF-harvest provider remains OFF/unavailable;
- unknown current/power values remain unknown;
- energy policy cannot mutate PacketId, delivery truth or durable-message semantics;
- no high-frequency power samples are written to flash.

### 5. LoRa transport wrapper + airtime gate

```text
components/mog_transport_lora/
components/mog_airtime/
```

Wrap the foundation SX1262 path without changing baseline behavior. Every LoRa TX passes AirtimeManager.

### 6. Neighbor + HybridRouter seam

```text
components/mog_neighbor/
components/mog_routing/
components/mog_route_discovery/
```

Use one routing-state writer/task. First target is behavior-equivalent LoRa-only forwarding. HybridRouter receives normalized energy cost/budgets only; it never reads board power registers or controls harvesting hardware.

### 7. Reliability + dedup

```text
components/mog_reliability/
components/mog_dedup/
```

Own end-to-end ACK correlation, bounded retries, jitter/backoff, delivery timeout, duplicate collapse and MessageStore handoff. Energy-driven deferral remains a real pending state and cannot appear as Delivered.

### 8. Multipath/failover + energy-aware route scoring

```text
components/mog_multipath/
components/mog_route_score/
components/mog_path_diversity/
```

Maintain a small bounded candidate set. Attempt a fresh cached alternate before broad rediscovery. Preserve one PacketId across failover.

Energy cost is one score input beside reliability, latency, airtime, congestion, freshness, stability and diversity. Add hysteresis so small energy fluctuations do not cause route flapping. Low-battery handhelds may reduce opportunistic relay/multipath work while preserving user-originated communication according to policy.

### 9. ESP-NOW transport including Long Range

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

EnergyManager provides background discovery/radio budgets; the adapter does not invent a second power-policy engine.

### 10. Event-driven delayed delivery

A durable message in WAITING_ROUTE becomes immediately eligible for bounded retry when credible connectivity returns, including:

```text
NEIGHBOR_UP
LINK_RECOVERED
ROUTE_DISCOVERED
ROUTE_AVAILABLE
TRANSPORT_RECOVERED
```

There is no fixed 200 m / 500 m / 1 km trigger. The trigger is usable connectivity evidence. Anti-storm jitter/backoff and airtime/energy policy may defer by a small bounded amount, but the user never needs to press Send again.

### 11. Health + metrics

```text
components/mog_health/
components/mog_metrics/
components/mog_rf_intelligence/
```

Expose queue pressure, route churn, retries, link recovery, storage health, airtime pressure, peer pressure, memory high-water marks, energy-state transitions, energy-policy deferrals and invalid/low-confidence power samples.

### 12. UI integration

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
Energy saving
Survival mode
Energy assist active   # only if compatible detected hardware exists
```

The UI never requires normal users to choose LoRa, ESP-NOW Normal, ESP-NOW LR, next hop, retry count, route number, MPPT point, rectifier threshold or energy-reservoir voltage.

### 13. Ambient RF Energy Assist provider seam

Create the optional LAB boundary in the same code project:

```text
components/mog_energy_rf_harvest/
  CMakeLists.txt
  include/mog_energy_rf_harvest.h
  mog_energy_rf_harvest.c/.cpp
  mog_energy_rf_provider.c/.cpp
```

First implementation is a simulator/mock/unavailable provider proving the boundary. Real hardware support is enabled only when compatible external rectenna/harvester-PMIC/storage hardware exists.

If future hardware exposes a reservoir/supercap, implement `TX_RESERVE_READY` only from measured threshold/hysteresis data. This event never bypasses ReliabilityManager or AirtimeManager.

Preferred physical assumption is a separate harvesting antenna/rectenna. Do not implement shared-antenna switching logic as a stable default without measured RF isolation/desense/insertion-loss evidence.

### 14. Optional/lab transports

Only after stable-core evidence:

```text
mog_transport_nan
mog_rf_assist
future external backscatter adapter
```

These remain removable without breaking LoRa-only operation.

## Missing-code implementation rule

If a required Firmware-mash component, adapter, driver shim, policy engine, simulator model or hardware-provider interface does not already exist upstream, implement it in-project under the appropriate `mog_` component rather than dropping the feature.

Rules:

- prefer documented ESP-IDF/vendor APIs and datasheet-defined interfaces;
- wrap vendor drivers rather than forking them when practical;
- write original integration/policy code when no compatible implementation exists;
- keep provenance and license notices for reused code;
- do not copy incompatible-license implementations into the project;
- add host/simulator tests for newly written policy/state-machine code;
- hardware-specific code must expose a safe `unavailable` path when hardware is absent;
- never invent custom cryptographic primitives; use vetted existing cryptographic implementations from the selected foundation/SDK.

A missing implementation is an engineering task, not permission to silently remove an approved capability.

## Dependency direction

```text
UI/Application
  -> Reliability / Message service
  -> HybridRouter <---- EnergyPolicySnapshot
  -> Transport interfaces
  -> Radio/board drivers

Board/PMIC telemetry ---> EnergyManager ---> EnergyPolicySnapshot
Optional RF harvest HW -> EnergyManager
```

MessageStore is below reliability and does not make routing decisions. Transport adapters report link evidence and transmit frames; they never own route tables. Energy providers report measurements only and never own routing or delivery policy.

## Concurrency model

- `network_task`: sole writer of routing/neighbor/reliability state;
- radio callbacks: enqueue small bounded events and return;
- storage worker: bounded durable I/O requested through explicit commands/events;
- energy worker/timer: samples providers at bounded cadence and publishes normalized snapshots/events;
- UI task: reads snapshots/events, never mutates routing internals;
- no hidden second routing task inside an adapter;
- no hardware-provider callback directly mutates routing state.

## Memory policy

- fixed/bounded pools for packets, events, ACKs, peers, routes and queued work;
- avoid unbounded target-side container growth;
- avoid per-packet heap allocation where practical;
- keep transient routes/metrics/energy telemetry in RAM/PSRAM;
- persist only recovery-critical state;
- do not persist high-frequency energy samples.

Exact capacities are measured and frozen after baseline RAM/PSRAM/flash evidence.

## Definition of implementation-ready

Coding may proceed once the baseline gate is PASS and these contracts remain internally consistent. No coding task should need to reopen ownership, transport hierarchy, retry-trigger semantics, no-SD policy, PacketId semantics, EnergyManager ownership or stable-vs-lab boundaries.