# Roadmap

## Phase 0 — Baseline and evidence

Select/pin the foundation, reproduce a T-Deck Plus build, run tests/simulator, record RAM/PSRAM/flash and establish LoRa-only behavior. No production routing rewrite until the baseline is reproducible and `BASELINE_APPROVED` is valid.

## Phase 1A — Standalone internal persistence

Implement the bounded internal MessageStore/no-SD durability foundation. Isolate identity/config from queue churn; prove pending messages survive reboot/power loss and define full-store/corruption/wear diagnostics.

## Phase 1B — EnergyManager foundation

Add one device energy-policy authority with measured battery/USB telemetry, hysteretic states and normalized `EnergyPolicySnapshot`. External harvesting remains optional/LAB.

## Phase 2 — HybridRouter skeleton, LoRa only

Introduce one routing abstraction without changing observable baseline LoRa behavior. Add event ownership, metrics and bounded structures first.

## Phase 3 — Multipath and deterministic failover

Add a small bounded route set, diversity scoring and cached-alternate failover before broad rediscovery. Tune from simulator/hardware evidence, not guessed permanent weights.

## Phase 4 — Reliability hardening

Unify end-to-end ACK semantics, retries/backoff, dedup, route-failure evidence and sender-side durable WAITING_ROUTE. A queued message retries when credible connectivity returns; no fixed-distance rule.

## Phase 4B — Opportunistic store-carry-forward custody

After sender-side delayed delivery is reliable, add optional BETA custody under `STORE_CARRY_CUSTODY_CONTRACT.md`/ADR 0008.

Sequence:

1. add bounded custody state/policy integrated with ReliabilityManager;
2. extend MessageStore with optional custody metadata;
3. require durable relay commit before acceptance evidence;
4. preserve same PacketId/end-to-end envelope/chat;
5. implement replay/dedup/TTL/energy/storage rejection policy;
6. prove relay reboot recovery;
7. prove sender can power off after accepted custody and destination later receives;
8. prove custody OFF leaves ordinary WAITING_ROUTE unchanged.

Custody acceptance never equals Delivered. Initial ownership/replication remains explicitly bounded.

## Phase 5A — ESP-NOW local lane

Add ESP-NOW Normal/LR as modes of one optional transport adapter, with bounded peers, coexistence scheduling and automatic fallback.

## Phase 5B — Smartphone-like T-Deck experience

Integrate Home, Messages, Contacts, Network and Settings. One contact remains one continuous chat regardless of LoRa/ESP-NOW/IP/custody path. Normal users never select transport/next-hop/retry internals.

## Phase 5C — Ambient RF Energy Assist software seam

Implement the optional harvesting-provider interface/mock and diagnostics. Physical RF harvesting remains LAB until compatible hardware is measured; stock T-Deck operation never depends on it.

## Phase 5D — IP backhaul and gateway federation

Add optional Internet-assisted delivery under HybridRouter:

1. `mog_transport_ip` + host/mock;
2. bearer-neutral NetifProvider;
3. stock T-Deck Wi-Fi provider;
4. bounded GatewayManager + authenticated/expiring discovery;
5. outbound authenticated federation session;
6. LoRa -> gateway -> IP -> gateway -> LoRa with same PacketId/chat;
7. IP loss -> radio alternate or WAITING_ROUTE;
8. IP recovery -> automatic queued retry;
9. multiple federation/bootstrap peers and failure tests;
10. cellular PPP provider only against selected real modem hardware.

Wi-Fi/cellular are bearers, not separate chats or routing engines. No single mandatory cloud server owns conversation history.

## Phase 6 — Integrated field beta

Run real T-Deck acceptance for no-SD durability, re-entry delivery, four-node failover, ESP-NOW/LoRa, Wi-Fi IP federation, UI and energy behavior. Custody beta additionally requires physical carry/disconnection, sender power-off, relay reboot and final exactly-once delivery tests.

Capture RSSI/SNR, airtime, recovery, storage health, memory high-water, UI responsiveness and power impact. Cellular/RF-harvest remain target-specific until hardware evidence exists.

## Phase 7 — Scale research

Simulate 50/100/500/1000+ node scenarios. Consider hierarchical/regional routing, gateway summaries, federation traffic aggregation and scheduling only when measured evidence justifies them. Include custody/storage/energy pressure rather than assuming unlimited relay capacity.

## Phase 8 — Lab-only transports and hardware research

Evaluate Wi-Fi Aware/NAN, advanced NAT traversal, RIS/passive RF assist, external backscatter and real ambient-RF harvesting hardware. Each remains independently removable.

## Phase 9 — Release engineering / one-flash package

After the selected feature tier passes its required evidence:

1. build from clean pinned checkout;
2. run required config/test gates;
3. generate merged/supported flash artifacts from real partition metadata;
4. generate `release-manifest.json` and SHA-256 hashes;
5. generate one-click/web-flasher descriptor where supported;
6. verify exact recovery path;
7. reject STABLE publication when required hardware evidence is missing.

Follow `FLASHER_RELEASE_CONTRACT.md` and issue #16. Normal users never manually assemble binaries or calculate offsets.

## Final implementation trigger

`CODING_TRIGGER_CONTRACT.md` is the authoritative future start point. It refuses production implementation without valid baseline evidence and a contradiction-free final controller review.

## Release channels

- **STABLE:** no-SD standalone T-Deck, internal durable queue, LoRa backbone, validated reliability/multipath, smartphone UI, EnergyManager and exact release/recovery package; no Internet/harvester dependency.
- **BETA:** STABLE foundation plus individually hardware-tested ESP-NOW, IP/gateway and/or custody capabilities.
- **LAB:** cellular targets without completed evidence, NAN/NAT experiments, RF assist/backscatter and Ambient RF Energy Assist hardware research.
