# Real-World Competitor Review — 2026-09-17

## Review posture

This review deliberately assumes a competing embedded/networking team is trying to prove Firmware-mash is not ready for real-world use. The goal is to identify failure modes before implementation, then review the proposed fixes a second time so we do not replace one weakness with unnecessary complexity.

## Real-world verdict

**CONDITIONAL GO.** The architecture is credible for low-rate off-grid messaging on stock T-Deck Plus hardware, but it is not yet evidence-backed for large-scale deployment. The biggest remaining practical gap is not another transport: it is durable standalone operation across reboot, power loss, congestion and low battery.

The T-Deck Plus hardware envelope is sufficient for the intended stable core if resource budgets remain bounded: 16 MB flash and 8 MB PSRAM provide enough room for firmware plus internal persistence, subject to the actual baseline image size and partition measurements.

## First-pass competitor findings

### 1. Current foundation mailbox durability is not enough

At the audited Bramble snapshot, the core mailbox component is a fixed RAM structure (`MAILBOX_MAX_ENTRIES = 32`) initialized with `memset()` and the mesh glue stores/retrieves directly from it. That means this store-and-forward layer by itself is volatile across reboot/power loss.

**Improvement:** Firmware-mash must add a durable internal `MessageStore` below/around the logical store-and-forward state. RAM mailbox structures become a hot cache/index, not the only copy of undelivered user data.

### 2. MicroSD dependency would make the product fragile

A removable card can be absent, corrupted or removed. Core messaging cannot depend on it.

**Improvement:** no-SD operation is a hard architecture and acceptance requirement. Internal flash holds all mandatory durable state. SD becomes optional bulk/diagnostic storage only.

### 3. Persisting everything would wear flash unnecessarily

A naive response to #1 would be writing routes, RSSI, retry counters and neighbor metrics on every event.

**Improvement:** persist only user/identity/recovery state that must survive reboot. Keep route/neighbor/RF measurements in RAM/PSRAM and reconstruct them naturally after boot.

### 4. LoRa airtime, not CPU speed, is the scale bottleneck

Multipath can improve resilience while simultaneously making a congested radio network worse if discoveries, retries and critical duplication are too aggressive.

**Improvement:** one central airtime manager, cached alternate paths before rediscovery, congestion as a route metric, bounded discovery and no default duplicate transmission for normal messages.

### 5. ESP-NOW helps locally but is not a magic range extender

It is useful as a nearby fast lane. It shares the ESP32-S3 2.4 GHz RF resources with Wi-Fi/BLE and channel/coexistence constraints matter.

**Improvement:** keep ESP-NOW optional and power-aware, with a bounded dynamic peer cache and a radio scheduler. Failure/removal must fall back to LoRa without changing the user workflow.

### 6. Portable real-world use requires power policy

Always-on scanning, bright display, GNSS and aggressive radio discovery can turn a technically correct mesh into a poor handheld product.

**Improvement:** measure battery drain per feature and add role-aware idle policies. A handheld endpoint and a fixed backbone node should not run identical discovery/display policies.

### 7. Power loss must be treated as normal, not exceptional

A handheld can die or reset during queue writes, compaction or updates.

**Improvement:** durable records need integrity/versioning and boot-time recovery. Queue corruption must be isolated from identity/configuration.

### 8. Safe update/recovery must be designed before flash becomes crowded

Adding OTA late can require a partition redesign that conflicts with data storage.

**Improvement:** reserve an A/B OTA-capable layout now, but do not enable OTA as a stable feature until signing, self-test confirmation and rollback/recovery are proven. USB remains the base recovery path.

### 9. Local diagnostics are mandatory when there is no phone/internet

A standalone off-grid device must explain what it is doing.

**Improvement:** local UI must expose at least radio state, neighbor count, selected/backup route summary, queued-message count, store health, battery and degraded adapters. Advanced metrics stay behind an expert view.

### 10. “Nationwide” must not mean one flat LoRa mesh

A single RF channel with uncontrolled discovery does not scale to arbitrary node counts.

**Improvement:** stable v1 targets measured local/regional networks. Hierarchy, regional summaries, channels/TDMA and backbone policy remain scale-research features triggered by simulator evidence.

## Second-pass review of those improvements

### A. Do not build a database on a microcontroller

The first proposal could drift toward a complex transactional storage engine.

**Better version:** use the simplest durable design that passes power-cut tests: a bounded append-oriented journal or proven fail-safe flash filesystem plus a compact RAM index. No query engine, no unbounded history.

### B. Do not persist complete route tables

Persisting routes sounds like faster boot but creates stale-path bugs and flash wear.

**Better version:** neighbors/routes rediscover after boot. Persist only exceptional information if later measurements prove it valuable. Security/identity and undelivered user messages have priority over route convenience.

### C. Do not spend all 16 MB on message history

A giant queue can steal future firmware/OTA headroom.

**Better version:** partition sizes are chosen after measuring the real baseline binary. Set explicit growth budgets for both app slots and a bounded message store. Old delivered history may be aggressively compacted/evicted; pending messages receive higher retention priority.

### D. Do not keep every radio awake merely to be “smart”

Hybrid routing can become a battery penalty.

**Better version:** transport availability is policy-driven. LoRa is always the stable backbone; ESP-NOW discovery windows can be adaptive. Optional transports are activated when their expected benefit exceeds energy/coexistence cost.

### E. Do not overreact to one failed transmission

Fast failover can become route flapping.

**Better version:** maintain failure confidence/hysteresis. Strong path-relevant errors can invalidate immediately; weak/noisy metrics degrade confidence gradually.

### F. Do not overcomplicate critical delivery

Sending every important message over two paths can congest the network.

**Better version:** critical duplicate-path delivery is exceptional and airtime-gated. Normal delivery uses one path and cached failover.

### G. Do not treat PSRAM as durable capacity

PSRAM is large but volatile.

**Better version:** use it for buffers, UI, route candidates and caches. Reboot correctness must rely only on internal flash plus deterministic reconstruction.

## New hard stable-core requirements

The stable line must therefore prove all of the following on a stock T-Deck Plus:

- no microSD installed;
- no phone or internet required for basic messaging;
- identity/config survive reboot;
- pending message survives reboot/power cycle;
- LoRa-only operation works;
- multi-hop failover works;
- no route causes an explicit queued/waiting state rather than silent loss;
- a damaged/incomplete queue record does not brick boot;
- bounded RAM/PSRAM and flash usage;
- deterministic full-store policy;
- USB recovery remains possible;
- optional ESP-NOW can be disabled without data-path redesign.

## Practical use case boundary

If these gates pass, Firmware-mash can be a credible real-world **text-oriented off-grid handheld/relay network**. It should not claim phone-like throughput, guaranteed long-distance delivery without relay coverage, or arbitrary national scale. Those depend on RF environment, infrastructure density, regional airtime constraints and measured deployment topology.

## Build-order change from this review

Before multipath implementation is considered complete, add a standalone persistence milestone:

1. reproduce baseline;
2. confirm no-SD baseline boot;
3. design/measure internal partition budget;
4. implement/reuse durable MessageStore abstraction;
5. power-cut/reboot recovery tests;
6. HybridRouter/multipath;
7. ESP-NOW hybrid lane;
8. field/battery validation.

This sequence prioritizes a device that keeps messages safe over one that merely discovers more routes.
