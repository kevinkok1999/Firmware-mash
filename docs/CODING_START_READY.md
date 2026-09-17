# Firmware-mash — Coding Start Ready

Date: 2026-09-17
Branch: `develop`

## Decision

`CODING_START=GO`

The three-phase preparation is complete: architecture contracts, ownership boundaries, phase gates, test entry points, target-build path and release/flasher fail-closed rules are defined. Coding may proceed in dependency order.

This marker does **not** claim that Phase 1, 2 or 3 implementation has already passed. Promotion remains evidence-gated.

## Current phase state

### Phase 1 — foundation/storage

Implementation present:
- pinned Bramble foundation and baseline marker;
- transactional dual-slot snapshot;
- append-only committed journal;
- bounded state/recovery engine;
- Bramble `msg_store_spiffs_*` compatibility adapter;
- proactive full-store rollover;
- bounded periodic checkpointing;
- durable-record layout compile-time guard;
- host/fault/reboot integration tests;
- pinned overlay and T-Deck target-build scripts.

Open evidence gate:
- repository issue #19: GitHub-hosted runners remain queued before step 1;
- therefore the current overlay still needs repository-local ESP-IDF/T-Deck target-build evidence before Phase-1 promotion.

Rule: Phase-2 code may be developed as isolated host-testable components, but target integration/promotion must not bypass Phase 1.

### Phase 2 — communication engine

Coding has started with the lowest-level host-testable primitives:
- `components/mog_core/`
  - origin-qualified logical message key;
  - durable reserve-ahead PacketId generator;
  - lifecycle terminal-state rules;
  - wrap-safe monotonic time helpers.
- `components/mog_events/`
  - bounded event queue;
  - telemetry-first shedding;
  - control-overflow resync requirement.
- `test/phase2/test_core/`
- `test/phase2/test_events/`
- `test/run_phase2_tests.sh` is the single fail-closed Phase-2 test entry point.

Network-wide message identity is `(origin identity, PacketId)`. The PacketId remains stable across retry/path/transport/reboot; unrelated origins are never deduplicated merely because their numeric PacketId matches.

Next implementation order:
1. complete core/events host verification;
2. conversation identity and same-chat ordering;
3. EnergyManager seam;
4. LoRa + AirtimeManager;
5. neighbor + HybridRouter LoRa-only;
6. reliability/E2E ACK/dedup/delayed delivery;
7. multipath;
8. custody/store-carry-forward;
9. ESP-NOW + RadioScheduler;
10. IP/gateway federation;
11. health/metrics/replay diagnostics.

### Phase 3 — product/release

Preparation complete:
- UI/product/recovery contracts exist;
- `test/run_phase3_tests.sh` fails closed until product suites and physical evidence exist;
- `scripts/release-package.sh` refuses to package around a red Phase-3 gate;
- release artifacts must use actual target-build offsets/manifests/hashes, never guessed flash addresses.

Implementation begins only after the communication core is sufficiently proven. STABLE remains impossible without explicit physical T-Deck evidence.

## Team review model

Every promoted layer is reviewed from three roles:
1. architecture/ownership;
2. reliability/fault recovery;
3. integration/release evidence.

No higher layer may mask a lower-layer failure.

## One-button end state

```text
phase1 PASS
 -> phase2 PASS
 -> phase3 PASS
 -> exact T-Deck build
 -> hardware evidence
 -> release manifest + hashes
 -> flasher manifest + recovery package
 -> one supported Flash action
```

Until those gates pass, generated development binaries must not be labeled STABLE or user-ready.
