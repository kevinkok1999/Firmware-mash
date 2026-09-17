# Firmware-mash — Three-Phase Implementation Plan

This is the mandatory build order for turning the approved architecture into a flashable T-Deck Plus product.

## Team model

Every change is reviewed from three roles before promotion:

1. **Architecture reviewer** — ownership boundaries, dependency direction, no duplicate router/store/energy authority, protocol compatibility.
2. **Reliability reviewer** — power loss, reboot, queue pressure, ACK/dedup, storage integrity, bounded retries and failure recovery.
3. **Integration/release reviewer** — pinned foundation, host/target builds, config matrix, reproducibility, artifact/flash/recovery correctness.

A phase passes only when all three concerns are satisfied. A later phase may be coded in isolation for host-testable primitives, but it may not be integrated/promoted to the target build until the previous phase gate is green. A higher layer may never hide or work around a lower-layer failure.

---

## Phase 1 — Foundation and durable core

**Goal:** make the floor strong enough that every later network feature can safely depend on it.

Required implementation/evidence:

- exact pinned Bramble foundation from `docs/BASELINE_APPROVED`;
- ESP-IDF/toolchain pin preserved;
- T-Deck Plus board target preserved;
- internal storage has no mandatory microSD dependency;
- `components/mog_message_store/` host-testable transaction/snapshot/journal/recovery logic;
- power-loss-safe compaction/rollover integrated below Bramble's existing `msg_store_spiffs_*` API;
- durable-record layout drift is compile-time guarded until an explicit versioned migration exists;
- host tests use `-Wall -Wextra -Werror`;
- fault tests cover torn slot, corrupt payload, wrong schema, journal tail repair, reboot recovery and fallback to a previous committed generation;
- target integration must never erase identity/config because message storage is unhealthy.

**Phase 1 gate:** `bash scripts/phase-gate.sh 1`

Promotion rule: Phase 2 target integration does not start until the Phase 1 gate passes in a clean checkout. Host-testable Phase-2 primitives may be developed in parallel, but they remain unintegrated/unpromoted while Phase 1 evidence is red. Physical power-cut and no-SD hardware evidence remains a STABLE release gate even after host/simulator tests pass.

Current external blocker: repository issue #19 tracks GitHub-hosted runners remaining queued before step 1. This is infrastructure evidence, not a reason to weaken the Phase-1 gate.

---

## Phase 2 — Communication engine

**Goal:** build the complete transport-neutral communication core on the proven foundation.

Build order inside the phase:

1. `mog_core` / PacketId / lifecycle / monotonic-time / event ownership;
2. `mog_conversation` + `mog_messaging` identity and same-chat continuity;
3. `mog_energy` policy seam;
4. LoRa transport + single AirtimeManager ownership;
5. neighbor state + HybridRouter LoRa-only seam;
6. ReliabilityManager: E2E ACK/retry/dedup/delayed delivery;
7. multipath and route scoring with hysteresis;
8. custody/store-carry-forward on durable storage;
9. ESP-NOW Normal/LR behind one adapter and RadioScheduler;
10. IP backhaul + Wi-Fi NetifProvider + GatewayManager/federation;
11. cellular NetifProvider only after selected modem hardware is proven;
12. health/metrics/replay hooks required for field diagnosis.

Hard invariants:

- one logical `HybridRouter`;
- one logical PacketId for one message across retries/failover/transports;
- one continuous conversation independent of route/transport;
- one logical network writer;
- all LoRa airtime passes through AirtimeManager;
- link transmit success is never user-visible delivery success;
- only destination E2E ACK may mark `Delivered`;
- no custom cryptographic primitive;
- all queues/pools/retries/custody storage are bounded;
- optional transports must never become dependencies of `CFG-LORA-STABLE`.

Minimum Phase 2 build profiles:

- `CFG-LORA-STABLE`
- `CFG-HYBRID-BETA`
- `CFG-CUSTODY-BETA`
- `CFG-IP-BETA`

**Phase 2 gate:** `bash scripts/phase-gate.sh 2`

The gate intentionally remains red until these components and their tests actually exist. `test/run_phase2_tests.sh` is the single Phase-2 test entry point and must fail closed when mandatory suites/configs are absent.

---

## Phase 3 — Product, UI, recovery and flasher

**Goal:** turn the proven communication engine into a safe end-user product and a reproducible flash package.

Required product work:

- smartphone-like Messages / Contacts / Network / Settings UX;
- conversation continuity across all transports;
- first-run region/identity setup;
- Advanced Diagnostics separated from normal UX;
- signed OTA/recovery policy and safe schema migration;
- normal update preserves identity/config/messages;
- target firmware builds from the pinned foundation plus Firmware-mash overlay;
- exact partition/flash offsets come from the target build, never handwritten guesses;
- release manifest, hashes, flasher manifest and recovery instructions generated automatically;
- wrong board/region/layout fails closed before flashing;
- BETA/LAB cannot be mislabeled STABLE;
- STABLE requires physical T-Deck evidence including no-SD boot, reboot/power-loss persistence, radio send/receive, recovery and update/rollback.

**Phase 3 gate:** `bash scripts/phase-gate.sh 3`

`test/run_phase3_tests.sh` and `scripts/release-package.sh` are the only approved product/release entry points. Both must fail closed until their required evidence exists. Only after Phase 3 passes may `.github/workflows/release-package.yml` produce a user-facing flash artifact.

---

## Preparation status vs implementation status

The **three-phase preparation is complete** when the contracts, build order, gates, test entry points and release rules are present. That means coding can begin.

It does **not** mean Phases 1–3 implementation have already passed. Implementation promotion remains evidence-based:

- Phase 1 becomes green only after its current overlay actually executes host + T-Deck target gates;
- Phase 2 becomes green only after the communication engine/tests/config matrix pass;
- Phase 3 becomes green only after product/release/hardware evidence passes.

This distinction prevents a planning-complete project from being mislabeled as a tested firmware release.

---

## One-button end state

The intended final flow is:

```text
clean checkout
  -> phase 1 gate
  -> phase 2 gate
  -> phase 3 gate
  -> exact T-Deck Plus build
  -> tests/evidence checks
  -> release manifest + SHA256SUMS
  -> flasher manifest + firmware binaries + recovery instructions
  -> one supported Flash action
```

The automation must fail closed. A green UI, successful socket write, generated `.bin`, or completed packaging step alone is never sufficient proof that a STABLE device is safe to flash.
