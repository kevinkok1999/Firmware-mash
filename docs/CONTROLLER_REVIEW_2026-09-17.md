# Controller Review — 2026-09-17

## Purpose

Independent red-team pass over the preparation set before production coding. The goal is to find contradictions, missing implementation decisions and places where a coding agent could otherwise improvise architecture.

## Findings resolved in this pass

### C1 — Delayed-delivery event names were inconsistent

Problem: delayed-delivery documents referenced `LINK_RECOVERED`, `ROUTE_AVAILABLE` and `TRANSPORT_RECOVERED`, while the central API event list did not define them.

Fix: `API_CONTRACTS.md` now includes the recovery events and explicitly states that they may make WAITING_ROUTE messages retry-eligible through ReliabilityManager.

Status: RESOLVED.

### C2 — ESP-NOW Long Range was described but not represented in the API capability model

Problem: the architecture intended ESP-NOW LR but a coding agent could have implemented it as a second routing stack or omitted an explicit capability.

Fix: one `MOG_LINK_ESPNOW` adapter now has Normal/LR capability flags and AUTO/NORMAL/LR mode semantics. LR remains baseline/toolchain/hardware-gated before STABLE.

Status: RESOLVED.

### C3 — Delivery state vocabulary differed between documents

Problem: `FAILED_PERMANENT`/`CREATED` appeared in delivery semantics but not the central MessageStore state list.

Fix: API state vocabulary aligned.

Status: RESOLVED.

### C4 — Wire compatibility was under-specified

Problem: routing/storage/transport design was detailed, but exact layering rules for preserving the foundation security envelope, PacketId across transports, fragmentation and versioning were not centralized.

Fix: added `WIRE_PROTOCOL_CONTRACT.md`.

Status: RESOLVED, with exact byte layout intentionally baseline-dependent.

### C5 — Smartphone UX had no explicit roadmap/test gate

Problem: UI could have been treated as cosmetic work after networking, risking a technically capable but difficult product.

Fix: added `UI_UX_CONTRACT.md`, Phase 5B roadmap milestone and UX test IDs in `TEST_TRACEABILITY.md`.

Status: RESOLVED.

### C6 — One-shot implementation lacked a single execution contract

Problem: a future coding agent would need to infer document precedence and problem-resolution policy.

Fix: added `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` with source-of-truth precedence, coding order, self-repair loop and controller review requirements.

Status: RESOLVED.

### C7 — Feature omission risk

Problem: capabilities were spread across architecture/roadmap/reviews.

Fix: added `FEATURE_MANIFEST.md` as one STABLE/BETA/LAB checklist.

Status: RESOLVED.

### C8 — Requirements were not fully traceable to tests

Problem: implementation could claim completion without corresponding test evidence.

Fix: added `TEST_TRACEABILITY.md` with stable test IDs and mapped owners/evidence.

Status: RESOLVED.

## Deliberately unresolved until measured baseline

These are not design omissions and must not be guessed:

1. exact pinned foundation commit at implementation start if upstream has moved;
2. actual firmware binary size;
3. actual internal RAM/PSRAM high-water marks;
4. final partition sizes;
5. exact MessageStore backend (LittleFS versus purpose-built journal) after measurement/fault testing;
6. final bounded capacities for route/path/peer/event/packet pools;
7. exact upstream wire header/identity representation and any compatible extension bytes;
8. actual ESP-NOW LR behavior/range/power on the specific T-Deck Plus build;
9. real battery runtime/thermal/coexistence results;
10. hardware evidence required to promote ESP-NOW from BETA to STABLE.

Filling these with invented numbers would make the preparation worse, not more complete.

## Controller verdict

Architecture/design preparation is CODE-READY SUBJECT TO BASELINE EVIDENCE.

Once the baseline gate is genuinely PASS, a coding agent should not need to redesign:

- routing ownership;
- no-SD policy;
- PacketId/delivery semantics;
- delayed-delivery trigger behavior;
- MessageStore responsibilities;
- multipath/failover ordering;
- ESP-NOW Normal/LR relationship;
- transport hierarchy;
- smartphone-like UI behavior;
- build/release tiers;
- test ownership;
- one-flash release expectation.

Any new contradiction discovered during coding must be fixed at the owning contract/ADR before implementation continues.