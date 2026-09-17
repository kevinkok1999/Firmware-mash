# Final Controller Review — 2026-09-17

controller_status=READY_FOR_CODING
unresolved_architecture_contradictions=0
coding_start_blocked_by_baseline=YES
actions_runner_status=QUEUED_NO_STEPS
release_package_status=CONTRACT_READY_IMPLEMENTATION_PENDING

## Purpose

This is the final pre-code contradiction audit after the three preparation phases. `READY_FOR_CODING` means the architecture, ownership rules, dependency order and evidence model are internally coherent. It does **not** override the baseline gate: production source still may not start until `docs/BASELINE_APPROVED` exists with genuine PASS evidence.

## Phase 1 — architecture completeness

### Findings resolved

1. **Sender delayed delivery versus true store-carry-forward was ambiguous.**
   - Resolved by `STORE_CARRY_CUSTODY_CONTRACT.md` and ADR 0008.
   - Custody acceptance requires durable relay commit.
   - Custody acceptance never equals destination delivery.
   - Same PacketId, protected envelope and chat survive custody transfer.
   - Initial ownership/replication is bounded; epidemic replication is forbidden.

2. **One-click/flasher outcome was described but not normatively specified.**
   - Resolved by `FLASHER_RELEASE_CONTRACT.md`.
   - Exact offsets/partition data come from real build output.
   - Manifest, SHA-256, flasher descriptor and recovery path are mandatory release outputs.
   - STABLE publication fails closed when evidence is incomplete.

3. **There was no single authoritative future coding trigger.**
   - Resolved by `CODING_TRIGGER_CONTRACT.md`.
   - The trigger checks baseline, controller status and source-of-truth contracts before production source creation.

### Phase 1 verdict

PASS. No unresolved ownership or delivery-truth contradiction remains.

## Phase 2 — implementation order, test traceability and automation

### Findings resolved

1. **Custody implementation order conflicted between roadmap and initial trigger draft.**
   - Resolved: custody core is now built after proven Reliability/MessageStore/Multipath and before optional ESP-NOW/IP complexity.
   - This isolates ownership/durability bugs from transport bugs.

2. **New custody/release/trigger requirements were not fully mapped to tests.**
   - Resolved by CUS-001..010, TRIG-001..003 and RELSE-001..008 in `TEST_TRACEABILITY.md`.

3. **Build isolation for custody did not exist.**
   - Resolved by `CFG-CUSTODY-BETA` plus custody-OFF stable regression.

4. **Prebuild checklist lacked IP/gateway/custody/release safety gates.**
   - Resolved in `PREBUILD_CHECKLIST.md`.

5. **CI did not require the new contracts.**
   - Resolved: preflight requires custody, coding-trigger, flasher and ADR 0008 documents.

6. **No one-click readiness/release workflow scaffolds existed.**
   - Added `.github/workflows/coding-readiness.yml`.
   - Added `.github/workflows/release-package.yml`.
   - Both fail closed when their prerequisites are absent.
   - `coding-readiness` validates readiness; a coding agent performs the implementation.
   - `release-package` eventually calls `scripts/release-package.sh`, which must be implemented during the coding/release phase from real build metadata.

### Phase 2 verdict

PASS. Requirements, build configurations, test IDs and workflow gates are aligned.

## Phase 3 — final red-team / release audit

### Architecture invariants rechecked

PASS:

- exactly one `HybridRouter` routing authority;
- exactly one `EnergyManager` power-policy authority;
- one logical PacketId across LoRa, ESP-NOW, IP, retries, reboot and custody;
- one contact remains one chat regardless of path;
- link/socket/gateway/custody success is never destination Delivered;
- MessageStore is internal-flash/no-SD core storage;
- custody durable commit precedes responsibility acceptance;
- all LoRa TX remains behind AirtimeManager;
- Wi-Fi/cellular are NetifProvider bearers, not separate routers/chats;
- GatewayManager/Discovery provide reachability evidence, not route ownership;
- no single mandatory cloud service owns conversation history;
- RF harvesting is energy-only and optional hardware;
- optional transports/providers/custody can be removed while stable LoRa sender-delayed-delivery remains;
- no custom cryptographic primitives;
- normal UI hides engineering internals;
- release layout is generated/measured, never guessed.

### Failure isolation rechecked

PASS by contract; implementation evidence is still future work:

- ESP-NOW failure -> LoRa remains;
- Internet/gateway failure -> radio or WAITING_ROUTE remains;
- cellular failure -> other paths remain;
- harvester absence/failure -> normal battery operation remains;
- custody disabled/rejected -> sender retains ordinary delayed delivery;
- storage pressure -> deterministic reject/eviction policy, never silent false acceptance;
- duplicate mixed paths -> dedup/exactly-once app presentation;
- bootstrap loss -> existing peers/local mesh remain by design;
- bad release evidence -> STABLE package creation rejected.

## Current blockers that are intentionally NOT resolved by design guessing

These are measurement/infrastructure gates, not architecture defects.

### B1 — baseline approval marker absent

Current state: `docs/BASELINE_APPROVED` does not exist on `develop`.

Required before production source:

- pinned foundation build evidence;
- host/unit PASS;
- simulator PASS;
- license review PASS;
- no-SD source/dependency review PASS;
- human-readable resource/build report.

The project already has strong upstream evidence for the pinned Bramble foundation, but the local gate remains deliberately fail-closed until the approved evidence path is recorded.

### B2 — GitHub Actions hosted runner not executing jobs

Latest observed project preflight remained queued before any workflow step executed. This is classified as CI/infrastructure state, not a firmware/compiler failure.

Do not mark local CI PASS until a runner actually executes the workflow or equivalent reproducible evidence is captured by an approved alternative path.

### B3 — exact resource/partition values

Still measurement-dependent:

- final app binary size;
- RAM/PSRAM high-water values;
- final packet/event/route/peer/gateway/custody pool capacities;
- final 16 MB partition layout;
- final MessageStore backend;
- exact merged-image/flash offsets;
- A/B OTA feasibility after real layout measurement.

### B4 — hardware promotion evidence

Still legitimately future:

- real no-SD cold boot/message recovery;
- LoRa field behavior;
- ESP-NOW Normal/LR coexistence/range/power;
- real Wi-Fi/gateway federation;
- custody carry/reboot/delivery;
- selected cellular modem/eSIM/operator combination;
- RF-harvester power/RF coexistence;
- final one-click flash + recovery acceptance.

These cannot be made “100%” by documentation alone and must remain evidence-gated.

## Authoritative coding order

Use `CODING_TRIGGER_CONTRACT.md`, `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` and `IMPLEMENTATION_BLUEPRINT.md`.

Condensed order:

```text
baseline approval
-> foundation import/reproduction
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

After every major layer: compile -> mapped tests -> impacted integration tests -> CFG-LORA-STABLE regression -> resource review -> controller pass.

## One-click coding path

The intended preparation outcome is now explicit:

1. obtain genuine baseline evidence and create valid `docs/BASELINE_APPROVED`;
2. run the `coding-readiness` manual workflow or equivalent trigger check;
3. when green, instruct the coding agent to execute `CODING_TRIGGER_CONTRACT.md`;
4. coding agent continues through the one-shot runbook without reopening settled architecture;
5. ordinary failures are repaired in-loop;
6. only proven hardware/toolchain/access blockers stop progression.

The workflow itself validates readiness; it does not falsely claim GitHub Actions can autonomously write the project without an attached coding agent.

## One-click flasher/release path

After implementation/evidence:

1. `release-package` workflow is triggered for board/region/tier;
2. it calls the future real `scripts/release-package.sh`;
3. build/test/evidence gates run;
4. package produces real binary artifact(s), `release-manifest.json`, `SHA256SUMS`, `flasher-manifest.json` and recovery instructions;
5. a supported flasher uses generated layout metadata;
6. normal user presses Flash and completes only normal onboarding.

The release workflow currently fails intentionally because production source/release script/evidence do not yet exist. That is correct pre-code behavior.

## Final verdict

### Architecture

**READY FOR CODING.** No unresolved architecture contradiction found in this final pass.

### Production coding right now

**BLOCKED BY BASELINE EVIDENCE GATE.** This is intentional and protects the project from building thousands of lines of custom firmware on an unrecorded local baseline.

### Final flasher right now

**NOT YET A REAL FIRMWARE ARTIFACT.** The full release/flasher contract and one-click workflow scaffold are prepared, but the real image can only be generated after the firmware is implemented, compiled and validated.

### Quality target

The preparation is now designed so the next coding pass is deterministic, test-traceable and fail-closed. “100% correct” remains a release evidence goal, not a claim that can be made before actual compilation and hardware validation.
