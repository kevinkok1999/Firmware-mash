# Roadmap

## Phase 0 — Baseline and evidence

Select/pin the foundation, reproduce a T-Deck Plus build, run its tests/simulator, record RAM/PSRAM/flash and establish LoRa-only behavior. Confirm cold boot/local UI with no microSD installed. No routing rewrite until the baseline is reproducible.

## Phase 1A — Standalone internal persistence

Before adding routing complexity, make the T-Deck a trustworthy standalone device. Define the measured 16 MB partition budget and durable internal MessageStore; keep identity/config isolated from high-churn queue data; prove pending messages survive reboot/power loss without a microSD card. Add deterministic full-store, corruption-recovery and flash-wear diagnostics.

## Phase 1B — EnergyManager foundation

Add one device energy-policy authority before advanced routing depends on power state. Integrate reliable board battery/USB telemetry exposed by the selected foundation, explicit hysteretic energy states and a normalized `EnergyPolicySnapshot`. Keep all external harvesting providers OFF. Prove that energy-state transitions do not block boot, corrupt durable messages or cause UI/network deadlocks.

## Phase 2 — HybridRouter skeleton, LoRa only

Introduce the routing abstraction without changing observable LoRa behavior. Add event ownership, metrics and bounded data structures first. Goal: architectural seam with zero feature regression. HybridRouter may consume normalized energy-cost/budget information but never raw PMIC/ADC state.

## Phase 3 — Multipath and deterministic failover

Add a small route set, alternate-path learning, path-diversity scoring and failover before broad rediscovery. Prove in simulator that a primary relay can disappear and traffic recovers through an independent cached path without manual configuration. Add energy cost as one bounded score input and preserve hysteresis against both route flapping and energy-state flapping.

## Phase 4 — Reliability hardening

Unify end-to-end ACK semantics, bounded retry/backoff, dedup, route-failure evidence and durable store-and-forward transitions. Stress loss, duplicate paths, reboot/power-cut recovery, queue pressure and energy-deferred work. A queued message must become retry-eligible as soon as credible usable connectivity returns; no fixed-distance retry rule. Energy policy may defer work but cannot create false delivery state.

## Phase 5A — ESP-NOW local lane

Add ESP-NOW as an optional transport adapter with dynamic peer cache, link metrics, power-aware discovery and 2.4 GHz scheduling. Include Normal and Long Range capability/mode under the same adapter. First prove direct T-Deck-to-T-Deck operation, then hybrid ESP-NOW -> LoRa and LoRa -> ESP-NOW forwarding. EnergyManager supplies discovery/relay budgets; ESP-NOW remains removable.

## Phase 5B — Smartphone-like T-Deck experience

Integrate the technical stack into a simple phone-style interface with Home, Messages, Contacts, Network and Settings. Normal users must be able to open a conversation, type on the T-Deck keyboard and send without choosing transport, route, retry count or next hop. Queued messages remain visible and automatically transition to Delivered when connectivity returns. Energy saving/survival state uses simple language; raw power electronics remain Advanced-only.

## Phase 5C — Ambient RF Energy Assist software seam

Implement the optional `mog_energy_rf_harvest` provider boundary, simulator/mock provider and Advanced diagnostics hooks in the same code project, but keep the real provider disabled unless compatible external hardware exists. Model optional harvested-power and reservoir/TX-ready evidence without inventing measurements.

Physical integration remains LAB until a real rectenna/harvester-PMIC/storage design is measured. Preferred assumption: separate harvesting antenna/rectenna. Shared-antenna operation requires explicit RF isolation/desense/insertion-loss evidence.

## Phase 6 — Field beta

Run no-SD standalone, two-node re-entry/delayed-delivery, four-node failover, hybrid ESP-NOW/LoRa and smartphone-UX acceptance tests on real hardware. Capture RSSI/SNR, airtime, recovery time, internal-store health, memory high-water marks, UI responsiveness and battery/power impact. Exercise NORMAL/CONSERVE/CRITICAL/SURVIVAL state transitions on stock T-Deck telemetry. Stable remains LoRa-first; ESP-NOW remains beta until repeated hardware evidence is strong.

Ambient RF harvesting is not part of the normal field-beta claim unless real external harvesting hardware is separately available and instrumented.

## Phase 7 — Scale research

Use simulation to evaluate 50/100/500/1000+ node regional scenarios. Research hierarchical/regional routing, TDMA/backbone scheduling and route summaries only if measured control traffic requires them. Include energy-aware relay distribution in scale scenarios so critically low-battery handhelds are not selected as preferred infrastructure when better powered nodes exist.

## Phase 8 — Lab-only transports, RF assist and harvesting hardware

Evaluate Wi-Fi Aware/NAN, RIS/passive RF assist, compatible external backscatter hardware and real ambient-RF energy-harvesting hardware. Measure harvested power, storage charge behavior, communication impact, receiver desense/insertion loss and safe fallback. None of these is required for the stable core and each remains independently removable.

## Release channels

- **STABLE:** standalone no-SD T-Deck operation, internal durable queue, LoRa backbone, validated multipath/failover, reliability/store-forward, smartphone-like local UI and EnergyManager with no dependence on harvesting hardware.
- **BETA:** Stable + hardware-tested ESP-NOW Normal/LR hybrid lane.
- **LAB:** NAN, TDMA/regional experiments, RF assist, external backscatter and Ambient RF Energy Assist hardware integration.