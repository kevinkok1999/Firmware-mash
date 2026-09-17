# Roadmap

## Phase 0 — Baseline and evidence

Select/pin the foundation, reproduce a T-Deck Plus build, run its tests/simulator, record memory/flash and establish LoRa-only behavior. No routing rewrite until the baseline is reproducible.

## Phase 1 — HybridRouter skeleton, LoRa only

Introduce the routing abstraction without changing observable LoRa behavior. Add event ownership, metrics and bounded data structures first. Goal: architectural seam with zero feature regression.

## Phase 2 — Multipath and deterministic failover

Add a small route set, alternate-path learning, path-diversity scoring and failover before broad rediscovery. Prove in simulator that a primary relay can disappear and traffic recovers through an independent cached path without manual configuration.

## Phase 3 — Reliability hardening

Unify end-to-end ACK semantics, bounded retry/backoff, dedup, route-failure evidence and store-and-forward transitions. Stress loss, duplicate paths and queue pressure.

## Phase 4 — ESP-NOW local lane

Add ESP-NOW as an optional transport adapter with dynamic peer cache, link metrics and 2.4 GHz scheduling. First prove direct T-Deck-to-T-Deck operation, then hybrid ESP-NOW -> LoRa and LoRa -> ESP-NOW forwarding.

## Phase 5 — Field beta

Run four-node failover and hybrid acceptance tests on real hardware. Capture RSSI/SNR, airtime, recovery time, memory high-water marks and power impact. Stable remains LoRa-first; ESP-NOW remains beta until repeated hardware evidence is strong.

## Phase 6 — Scale research

Use simulation to evaluate 50/100/500/1000+ node regional scenarios. Research hierarchical/regional routing, TDMA/backbone scheduling and route summaries only if measured control traffic requires them.

## Phase 7 — Lab-only transports and RF assist

Evaluate Wi-Fi Aware/NAN, RIS/passive RF assist and compatible external backscatter hardware. None of these is required for the stable core and each remains independently removable.

## Release channels

- **STABLE:** LoRa backbone, validated multipath/failover, reliability and store-forward.
- **BETA:** Stable + hardware-tested ESP-NOW hybrid lane.
- **LAB:** NAN, TDMA/regional experiments, RF assist and external backscatter.
