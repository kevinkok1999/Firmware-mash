# Firmware-mash Architecture Contract

## Core invariant

There is exactly one logical routing authority: `HybridRouter`. Physical technologies are adapters below it, not independent user-facing mesh stacks. Device energy policy is owned separately by exactly one `EnergyManager`.

```text
Application / UI
  -> Message Service
  -> Security / Identity
  -> Reliability Manager
  -> HybridRouter <---------------- EnergyPolicySnapshot
  -> Transport Manager                     ^
       -> LoRaTransport (required)          |
       -> EspNowTransport (optional)        |
       -> IpTransport (optional/BETA)       |
            -> WiFi NetifProvider           |
            -> Cellular PPP NetifProvider  |
       -> NanTransport (experimental)       |
       -> BackscatterTransport (future)     |
  -> GatewayManager / GatewayDiscovery      |
  -> RF Intelligence / RF Assist            |
                                             |
Board battery/USB telemetry ----------> EnergyManager
External RF harvester provider (LAB) -> EnergyManager
```

`GatewayManager` and `GatewayDiscovery` provide authenticated capability/reachability evidence to `HybridRouter`; they are not independent routing authorities. Energy harvesting is not a transport and does not create a logical mesh hop. The complete energy contract is in `ENERGY_MANAGEMENT_CONTRACT.md` and ADR 0006. The complete IP/gateway contract is in `IP_GATEWAY_FEDERATION_CONTRACT.md` and ADR 0007.

## Stable core

The stable line must always support LoRa-only operation. Multipath, retries, queues and route tables are bounded. Routing state has a single writer (`network_task`); radio/network callbacks enqueue events and do not mutate routing tables directly.

A stock T-Deck Plus must also run the complete stable core **without a microSD card installed**, **without any energy-harvesting hardware installed** and **without Internet connectivity**. Internal flash is the durable source of truth; PSRAM/RAM are volatile working memory. microSD is optional bulk storage only.

## Logical packet and conversation model

A message keeps one logical packet identity across all physical transports. Transport-specific framing wraps the packet but does not create a new application identity. This is required for end-to-end ACK correlation, replay defense and deduplication when multiple paths exist.

Conversation identity belongs to the contact/identity layer, not to LoRa, ESP-NOW, Wi-Fi or cellular. Transport/path changes never create a second chat.

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
- `mog_transport_ip`: bearer-neutral IP backhaul adapter.
- `mog_netif_wifi`: Wi-Fi IP provider.
- `mog_netif_cellular`: optional modem/PPP IP provider.
- `mog_gateway`: gateway capability/reachability/session policy.
- `mog_gateway_discovery`: bounded authenticated gateway discovery/bootstrap.
- `mog_radio_scheduler`: 2.4 GHz coexistence scheduling.
- `mog_airtime`: centralized LoRa airtime/regulatory gate.
- `mog_rf_intelligence`: EWMA/rolling link quality and stability.
- `mog_energy`: device energy state, hysteresis, budgets and normalized policy snapshots.
- `mog_energy_rf_harvest`: optional/LAB external RF-harvesting provider.
- `mog_rf_assist`: future RIS/passive-RF advisory control.
- `mog_health`: queue, pool, route, radio, gateway and energy health monitoring.
- `mog_metrics`: consistent simulator/device metrics.

Existing foundation components should be extended rather than duplicated when that is cleaner and license-compatible.

## Standalone storage contract

The storage design follows ADR 0005:

- identity/security/configuration: internal NVS or equivalent small durable store;
- pending/store-forward user messages: separate bounded internal MessageStore;
- route/neighbor/RF/energy/gateway discovery history: RAM/PSRAM by default, reconstructed after reboot;
- microSD: never required for boot, identity, routing, undelivered core messages or recovery.

Exact partition sizes are not fixed until the pinned baseline image is measured. The 16 MB layout must preserve firmware growth/recovery margin and, if retained, safe A/B OTA space. Message-store corruption must not force identity/configuration loss.

## Multipath policy

A destination may keep a small bounded set of candidate paths. Initial design target: at most three, subject to memory validation. The primary path is used normally. Backups are ranked not only by quality but by independence from the primary path. Cached alternatives are attempted before broad route rediscovery.

A path may contain heterogeneous segments such as LoRa -> IP gateway -> remote LoRa while preserving one logical PacketId and delivery state.

## Route metrics

Candidate metrics may include ETX/PDR, hop count, RSSI/SNR, estimated airtime, latency, congestion, energy cost, metered-data cost class, gateway freshness/trust confidence, path scope, freshness, stability and path diversity. Fixed-point arithmetic is preferred in hot paths. Route-score weights must be versioned and tuned from simulator/hardware evidence rather than guessed once and hidden in code.

Energy cost comes from normalized EnergyManager/transport observations. HybridRouter does not read PMIC registers or control harvesting hardware directly. IP availability is not an automatic priority override: a reliable direct radio path may still win according to policy/score.

## Reliability

Transport-level success is not end-to-end delivery. `ReliabilityManager` owns application ACKs, bounded retransmission and failover. Delivery classes are planned as BEST_EFFORT, NORMAL and CRITICAL. Critical delivery may use an independent second path only when airtime, energy and policy budgets allow it.

A committed pending message must survive reboot/power interruption once the durable MessageStore milestone is implemented. Normal route optimization and gateway-discovery state do not need to survive reboot unless a later ADR proves otherwise.

## LoRa

LoRa/SX1262 is the required long-range backbone. Every LoRa transmission passes through one airtime gate that enforces regional configuration and retry/multipath budgets. No experimental module may transmit around it.

## ESP-NOW

ESP-NOW is an optional local fast lane. It requires a bounded/dynamic peer cache, sequence numbers, deduplication and application-level delivery evidence. It must be possible to disable it without breaking the LoRa mesh. Discovery/scanning must be power-aware because the ESP32-S3 shares one 2.4 GHz RF resource across Wi-Fi/ESP-NOW/BLE.

## IP backhaul and gateway federation

`mog_transport_ip` is one optional logical transport regardless of whether the IP bearer is Wi-Fi or cellular. A normal Wi-Fi access point supplies IP connectivity only after legitimate association; it is not an unconfigured Firmware-mash relay.

Cellular requires compatible modem hardware. The preferred implementation uses maintained ESP-IDF/vendor networking APIs and a replaceable PPP/netif provider rather than embedding modem-specific behavior in routing code.

`GatewayManager`/`GatewayDiscovery` may learn authenticated gateway capabilities from the mesh, local LAN discovery, configured bootstrap peers and already-authenticated federation peers. Discovery is bounded, expiring and versioned.

Federation is designed for multiple peers/entrypoints. One bootstrap endpoint may assist discovery, but loss of a single central service must not erase conversations or break already-known peers. Loss of all Internet paths leaves LoRa/ESP-NOW/store-forward operational.

Normal handhelds may initiate outbound secure sessions; public inbound listener behavior is a gateway-class role. IP sessions use maintained standard security libraries. Gateways relay protected logical envelopes and do not require plaintext user content.

## Energy management and RF harvesting

`EnergyManager` owns device energy policy. It consumes board battery/USB telemetry and optional provider samples, applies hysteresis and publishes an immutable `EnergyPolicySnapshot` to consumers.

Stable software may use energy state to tune display/GNSS/background discovery, relay willingness, IP keepalive/discovery intensity and multipath budget. It may not change PacketId, encryption boundaries, delivery truth or bypass ReliabilityManager/AirtimeManager.

Ambient RF harvesting requires external compatible hardware. The preferred physical assumption is a separate harvesting antenna/rectenna so the tuned EU868 communications path is not loaded. A shared-antenna design is LAB-only until measured insertion loss, matching, desense and TX isolation are acceptable.

Loss or removal of harvesting hardware must degrade to normal battery operation, never boot failure or LoRa loss. Ambient RF is an optional energy assist, not a guaranteed power source.

## RF assist and backscatter

A passive reflector or RIS is a physical-link assist, not a logical mesh hop. Future ambient/backscatter support requires compatible external RF hardware and will be represented as a separate transport only after real hardware exists. Standard T-Deck hardware is not assumed to become an ambient-backscatter radio through firmware alone.

RF **energy harvesting** is different from backscatter: it belongs under EnergyManager and never becomes a transport merely because it captures RF energy.

## Failure isolation

If ESP-NOW, IP backhaul, a gateway peer, cellular modem, NAN, RF-assist or an RF-harvest provider fails, only the affected adapter/provider/path is disabled/degraded and LoRa operation continues. Every queue, peer/session table, route set and packet pool must have an explicit overflow policy and metric. Silent failure is forbidden.

If the durable message store is damaged, recovery must isolate/discard only invalid/incomplete queue records where possible and preserve identity/configuration. A storage fault must not create a permanent boot loop.

Implausible energy samples must be rejected/low-confidence; an energy provider may not directly disable the required LoRa backbone. Malformed or unauthenticated gateway/federation input must fail closed without blocking the local network task.
