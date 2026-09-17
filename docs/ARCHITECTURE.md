# Firmware-mash Architecture Contract

## Core invariant

There is exactly one logical routing authority: `HybridRouter`. Physical technologies are adapters below it, not independent user-facing mesh stacks.

```text
Application
  -> Message Service
  -> Security / Identity
  -> Reliability Manager
  -> HybridRouter
  -> Transport Manager
       -> LoRaTransport (required)
       -> EspNowTransport (optional)
       -> NanTransport (experimental)
       -> BackscatterTransport (future external hardware)
  -> RF Intelligence / RF Assist (advisory, not routing authority)
```

## Stable core

The stable line must always support LoRa-only operation. Multipath, retries, queues and route tables are bounded. Routing state has a single writer (`network_task`); radio callbacks enqueue events and do not mutate routing tables directly.

A stock T-Deck Plus must also run the complete stable core **without a microSD card installed**. Internal flash is the durable source of truth; PSRAM/RAM are volatile working memory. microSD is optional bulk storage only.

## Logical packet model

A message keeps one logical packet identity across all physical transports. Transport-specific framing wraps the packet but does not create a new application identity. This is required for end-to-end ACK correlation, replay defense and deduplication when multiple paths exist.

## Planned core modules

- `mog_core`: initialization, common state, lifecycle.
- `mog_packet`: wire envelope and versioning.
- `mog_events`: bounded event queues.
- `mog_neighbor`: direct-neighbor and per-link metrics.
- `mog_routing`: HybridRouter ownership and forwarding decisions.
- `mog_route_discovery`: bounded reactive discovery.
- `mog_multipath`: small route sets and alternate paths.
- `mog_route_score`: versioned route scoring.
- `mog_path_diversity`: detect shared failure domains.
- `mog_reliability`: end-to-end ACK, retries and delivery classes.
- `mog_dedup`: duplicate/replay-facing packet cache.
- `mog_message_store`: durable bounded internal-flash queue for unavailable destinations.
- `mog_transport_lora`: SX1262 long-range adapter.
- `mog_transport_espnow`: ESP-NOW local lane.
- `mog_radio_scheduler`: 2.4 GHz coexistence scheduling.
- `mog_airtime`: centralized LoRa airtime/regulatory gate.
- `mog_rf_intelligence`: EWMA/rolling link quality and stability.
- `mog_rf_assist`: future RIS/passive-RF advisory control.
- `mog_health`: queue, pool, route and radio health monitoring.
- `mog_metrics`: consistent simulator/device metrics.

Existing foundation components should be extended rather than duplicated when that is cleaner and license-compatible.

## Standalone storage contract

The storage design follows ADR 0005:

- identity/security/configuration: internal NVS or equivalent small durable store;
- pending/store-forward user messages: separate bounded internal MessageStore;
- route/neighbor/RF history: RAM/PSRAM by default, reconstructed after reboot;
- microSD: never required for boot, identity, routing, undelivered core messages or recovery.

Exact partition sizes are not fixed until the pinned baseline image is measured. The 16 MB layout must preserve firmware growth/recovery margin and, if retained, safe A/B OTA space. Message-store corruption must not force identity/configuration loss.

## Multipath policy

A destination may keep a small bounded set of candidate paths. Initial design target: at most three, subject to memory validation. The primary path is used normally. Backups are ranked not only by quality but by independence from the primary path. Cached alternatives are attempted before broad route rediscovery.

## Route metrics

Candidate metrics may include ETX/PDR, hop count, RSSI/SNR, estimated airtime, latency, congestion, energy cost, freshness, stability and path diversity. Fixed-point arithmetic is preferred in hot paths. Route-score weights must be versioned and tuned from simulator/hardware evidence rather than guessed once and hidden in code.

## Reliability

Transport-level success is not end-to-end delivery. `ReliabilityManager` owns application ACKs, bounded retransmission and failover. Delivery classes are planned as BEST_EFFORT, NORMAL and CRITICAL. Critical delivery may use an independent second path only when the airtime budget allows it.

A committed pending message must survive reboot/power interruption once the durable MessageStore milestone is implemented. Normal route optimization state does not need to survive reboot.

## LoRa

LoRa/SX1262 is the required long-range backbone. Every LoRa transmission passes through one airtime gate that enforces regional configuration and retry/multipath budgets. No experimental module may transmit around it.

## ESP-NOW

ESP-NOW is an optional local fast lane. It requires a bounded/dynamic peer cache, sequence numbers, deduplication and application-level delivery evidence. It must be possible to disable it without breaking the LoRa mesh. Discovery/scanning must be power-aware because the ESP32-S3 shares one 2.4 GHz RF resource across Wi-Fi/ESP-NOW/BLE.

## RF assist and backscatter

A passive reflector or RIS is a physical-link assist, not a logical mesh hop. Future ambient/backscatter support requires compatible external RF hardware and will be represented as a separate transport only after real hardware exists. Standard T-Deck hardware is not assumed to become an ambient-backscatter radio through firmware alone.

## Failure isolation

If ESP-NOW, NAN or RF-assist fails, the adapter is disabled/degraded and LoRa operation continues. Every queue, peer table, route set and packet pool must have an explicit overflow policy and metric. Silent failure is forbidden.

If the durable message store is damaged, recovery must isolate/discard only invalid/incomplete queue records where possible and preserve identity/configuration. A storage fault must not create a permanent boot loop.
