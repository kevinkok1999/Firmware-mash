# Final Controller Review — 2026-09-17

controller_status=READY_FOR_CODING
unresolved_architecture_contradictions=0
coding_start_blocked_by_baseline=YES
actions_runner_status=QUEUED_NO_STEPS
release_package_status=CONTRACT_READY_IMPLEMENTATION_PENDING

## Purpose

Final pre-code contradiction audit after the three preparation phases. `READY_FOR_CODING` means architecture/ownership/dependency/evidence rules are internally coherent. It does **not** override the baseline gate: production source still may not start until `docs/BASELINE_APPROVED` exists with genuine PASS evidence.

## Phase 1 — architecture completeness

Resolved:

1. Sender delayed delivery vs true store-carry-forward ambiguity -> `STORE_CARRY_CUSTODY_CONTRACT.md` + ADR 0008.
2. One-click/flasher outcome lacked normative contract -> `FLASHER_RELEASE_CONTRACT.md`.
3. No single future coding entrypoint -> `CODING_TRIGGER_CONTRACT.md`.
4. Packet delivery and MessageStore explicitly separate custody acceptance from destination delivery.

Phase 1 verdict: **PASS**.

## Phase 2 — order, traceability and automation

Resolved:

1. Custody dependency-order conflict -> custody core now follows proven Reliability/MessageStore/Multipath and precedes optional ESP-NOW/IP complexity.
2. Added CUS-001..010, TRIG-001..003 and RELSE-001..008 traceability.
3. Added `CFG-CUSTODY-BETA` + custody-OFF stable regression.
4. Prebuild now covers gateway/custody/release safety.
5. Preflight requires custody, coding-trigger, flasher and ADR 0008 contracts.
6. Added fail-closed manual `coding-readiness` workflow.
7. Added fail-closed future `release-package` workflow scaffold.
8. Root README and docs index now point to the same final architecture/order.

Phase 2 verdict: **PASS**.

## Phase 3 — final red-team / release audit

### Architecture invariants rechecked

PASS:

- exactly one `HybridRouter` routing authority;
- exactly one `EnergyManager` power-policy authority;
- one PacketId across LoRa, ESP-NOW, IP, retries, reboot and custody;
- one contact remains one chat regardless of path;
- link/socket/gateway/custody success is never destination `Delivered`;
- MessageStore is internal-flash/no-SD core storage;
- custody durable commit precedes responsibility acceptance;
- all LoRa TX stays behind AirtimeManager;
- Wi-Fi/cellular are bearer providers, not separate routers/chats;
- GatewayManager/Discovery supplies reachability evidence, not route ownership;
- no single mandatory cloud service owns conversation history;
- RF harvesting is energy-only and optional hardware;
- optional transports/providers/custody can be removed while stable LoRa sender delayed delivery remains;
- no custom cryptographic primitives;
- normal UI hides engineering internals;
- release layout is generated/measured, never guessed.

### Failure isolation rechecked

Contract-level PASS; implementation evidence remains future work:

- ESP-NOW failure -> LoRa remains;
- Internet/gateway failure -> radio or WAITING_ROUTE remains;
- cellular failure -> other paths remain;
- harvester absence -> normal battery operation;
- custody disabled/rejected -> sender retains delayed delivery;
- storage pressure -> deterministic reject/eviction, never false acceptance;
- duplicate mixed paths -> dedup/exactly-once presentation;
- bootstrap loss -> local mesh/existing peers remain by design;
- missing release evidence -> STABLE packaging rejected.

## Current blockers intentionally not guessed away

### B1 — baseline marker absent

`docs/BASELINE_APPROVED` is currently absent on `develop`.

Required before production source:

- pinned foundation build PASS;
- host/unit PASS;
- simulator PASS;
- license PASS;
- no-SD dependency review PASS;
- human-readable measured resource/build report.

### B2 — GitHub Actions runner not executing jobs

Latest observed project preflight remained queued before any workflow step executed. This is CI/infrastructure state, not a firmware/compiler failure.

Do not call local CI PASS until a runner actually executes or equivalent approved reproducible evidence is captured.

### B3 — exact resource/partition values

Still measurement-dependent:

- final app binary size;
- RAM/PSRAM high-water;
- packet/event/route/peer/gateway/custody capacities;
- final 16 MB partition layout;
- MessageStore backend;
- exact merged-image offsets;
- A/B OTA feasibility.

### B4 — hardware promotion evidence

Still future/evidence-gated:

- real no-SD cold boot/message recovery;
- LoRa field behavior;
- ESP-NOW Normal/LR coexistence/range/power;
- real Wi-Fi/gateway federation;
- custody carry/reboot/delivery;
- selected cellular modem/eSIM/operator combination;
- RF-harvest power/RF coexistence;
- final one-click flash + recovery acceptance.

## Authoritative coding order

```text
baseline approval
-> foundation integration
-> MessageStore
-> packet/events/core
-> EnergyManager
-> LoRa/AirtimeManager
-> HybridRouter LoRa-only
-> Reliability/dedup/delayed delivery
-> Multipath/scoring
-> Custody core
-> ESP-NOW Normal/LR
-> IP/gateway federation
-> cross-transport recovery
-> health/metrics/controller review
-> smartphone UI
-> RF-harvest seam
-> optional lab features
-> release package/flasher
```

After each layer: compile -> mapped tests -> impacted integration tests -> CFG-LORA-STABLE regression -> resource review -> controller pass.

## One-click coding path

1. obtain genuine baseline evidence and create valid `docs/BASELINE_APPROVED`;
2. run `coding-readiness` workflow or equivalent trigger check;
3. when green, instruct coding agent to execute `CODING_TRIGGER_CONTRACT.md`;
4. agent runs one-shot implementation, repairing ordinary failures in-loop;
5. only proven hardware/toolchain/access blockers stop progression.

The workflow validates readiness; it does not pretend GitHub Actions writes the firmware by itself without a coding agent.

## One-click flasher/release path

After implementation/evidence:

1. trigger `release-package` for board/region/tier;
2. future real `scripts/release-package.sh` builds exact package;
3. required build/test/evidence gates run;
4. output contains binary artifact(s), `release-manifest.json`, `SHA256SUMS`, `flasher-manifest.json`, recovery instructions;
5. supported flasher uses generated layout metadata;
6. normal user performs only Flash + onboarding.

The release workflow currently fails intentionally because production source/release script/evidence do not exist yet. That is correct pre-code behavior.

## Final verdict

### Architecture

**READY FOR CODING — 0 unresolved architecture contradictions.**

### Production coding right now

**BLOCKED BY BASELINE EVIDENCE GATE.** This is intentional.

### Final flasher right now

**CONTRACT/WORKFLOW SCAFFOLD READY; REAL FIRMWARE ARTIFACT NOT YET BUILT.**

“100% correct” remains a release evidence target, not a claim made before compilation and real hardware validation.
