# Roadmap

## Phase 0 — Baseline and evidence

Select/pin the foundation, reproduce a T-Deck Plus build, run its tests/simulator, record RAM/PSRAM/flash and establish LoRa-only behavior. Confirm cold boot/local UI with no microSD installed. No routing rewrite until the baseline is reproducible.

## Phase 1 — Standalone internal persistence

Before adding routing complexity, make the T-Deck a trustworthy standalone device. Define the measured 16 MB partition budget and durable internal MessageStore; keep identity/config isolated from high-churn queue data; prove pending messages survive reboot/power loss without a microSD card. Add deterministic full-store, corruption-recovery and flash-wear diagnostics.

## Phase 2 — HybridRouter skeleton, LoRa only

Introduce the routing abstraction without changing observable LoRa behavior. Add event ownership, metrics and bounded data structures first. Goal: architectural seam with zero feature regression.

## Phase 3 — Multipath and deterministic failover

Add a small route set, alternate-path learning, path-diversity scoring and failover before broad rediscovery. Prove in simulator that a primary relay can disappear and traffic recovers through an independent cached path without manual configuration.

## Phase 4 — Reliability hardening

Unify end-to-end ACK semantics, bounded retry/backoff, dedup, route-failure evidence and durable store-and-forward transitions. Stress loss, duplicate paths, reboot/power-cut recovery and queue pressure. A queued message must become retry-eligible as soon as credible usable connectivity returns; no fixed-distance retry rule.

## Phase 5A — ESP-NOW local lane

Add ESP-NOW as an optional transport adapter with dynamic peer cache, link metrics, power-aware discovery and 2.4 GHz scheduling. Include Normal and Long Range capability/mode under the same adapter. First prove direct T-Deck-to-T-Deck operation, then hybrid ESP-NOW -> LoRa and LoRa -> ESP-NOW forwarding.

## Phase 5B — Smartphone-like T-Deck experience

Integrate the technical stack into a simple phone-style interface with Home, Messages, Contacts, Network and Settings. Normal users must be able to open a conversation, type on the T-Deck keyboard and send without choosing transport, route, retry count or next hop. Queued messages remain visible and automatically transition to Delivered when connectivity returns. Advanced diagnostics remain optional and separated from normal operation.

## Phase 6 — Field beta

Run no-SD standalone, two-node re-entry/delayed-delivery, four-node failover, hybrid ESP-NOW/LoRa and smartphone-UX acceptance tests on real hardware. Capture RSSI/SNR, airtime, recovery time, internal-store health, memory high-water marks, UI responsiveness and battery/power impact. Stable remains LoRa-first; ESP-NOW remains beta until repeated hardware evidence is strong.

## Phase 7 — Scale research

Use simulation to evaluate 50/100/500/1000+ node regional scenarios. Research hierarchical/regional routing, TDMA/backbone scheduling and route summaries only if measured control traffic requires them.

## Phase 8 — Lab-only transports and RF assist

Evaluate Wi-Fi Aware/NAN, RIS/passive RF assist and compatible external backscatter hardware. None of these is required for the stable core and each remains independently removable.

## Release channels

- **STABLE:** standalone no-SD T-Deck operation, internal durable queue, LoRa backbone, validated multipath/failover, reliability/store-forward and smartphone-like local UI.
- **BETA:** Stable + hardware-tested ESP-NOW Normal/LR hybrid lane.
- **LAB:** NAN, TDMA/regional experiments, RF assist and external backscatter.
