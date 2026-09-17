# Coding Trigger Contract

## Purpose

Provide one authoritative entrypoint for the future production implementation pass. The trigger is a contract for a coding agent, not permission to bypass the baseline gate.

## Required trigger condition

Production implementation starts only when:

- branch is `develop` or an explicitly derived implementation branch;
- `docs/BASELINE_APPROVED` exists and passes preflight marker checks;
- the final controller report has no unresolved architecture contradiction;
- required normative docs/ADRs are present;
- pinned foundation/toolchain/provenance data are available.

If these conditions are not satisfied, the trigger stops before creating production source.

## Authoritative instruction

> Execute Firmware-mash from the current repository contracts. Do not redesign settled architecture. Follow `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` and `IMPLEMENTATION_BLUEPRINT.md` in dependency order. Implement missing approved `mog_` code rather than omitting capabilities. After each major layer, compile, run mapped tests, run the LoRa-stable regression, record resource/evidence impact, repair ordinary failures in-loop, and continue. Stop only for a proven measurement/hardware/toolchain/access blocker. Never invent hardware validation, custom crypto or release evidence.

## Document precedence

1. accepted ADRs;
2. `ARCHITECTURE.md`;
3. packet/delivery/custody/IP/energy/storage contracts;
4. `API_CONTRACTS.md`;
5. routing/build/UI/security contracts;
6. `FEATURE_MANIFEST.md`;
7. `TEST_TRACEABILITY.md`;
8. implementation blueprint/runbook/roadmap;
9. review/evidence documents.

A contradiction is fixed at the owning contract before code proceeds.

## Implementation stages

1. pinned foundation integration;
2. internal durable MessageStore/no-SD;
3. packet/event/core primitives;
4. EnergyManager;
5. LoRa + AirtimeManager;
6. HybridRouter LoRa-only seam;
7. ReliabilityManager + dedup + sender delayed delivery;
8. bounded multipath/route scoring;
9. optional custody core/store-carry state machine on the proven LoRa/storage/reliability foundation;
10. ESP-NOW Normal/LR adapter;
11. IP backhaul + gateway federation;
12. cross-transport recovery/event integration for queued/custody-held messages;
13. health/metrics/controller checks;
14. smartphone-like UI integration;
15. optional Ambient RF provider seam;
16. optional lab transports/hardware providers;
17. release-engineering handoff.

Why custody precedes extra transports: its durability/ownership semantics must first be proven without mixing in Wi-Fi/ESP-NOW complexity. Later transports simply become additional forwarding opportunities for the same custody-held PacketId.

## Per-stage loop

```text
implement smallest coherent layer
-> compile target/config
-> run mapped unit/simulator tests
-> run impacted integration tests
-> run CFG-LORA-STABLE regression
-> inspect resource delta
-> inspect architecture/security boundary
-> repair failure
-> record evidence
-> continue
```

Ordinary compilation/test failures are engineering work, not a reason to ask the user to debug the implementation.

## Mandatory non-negotiables

- one `HybridRouter` routing authority;
- one `EnergyManager` power-policy authority;
- one logical PacketId across transports/retries/custody;
- one contact remains one conversation;
- end-to-end delivery evidence remains distinct from link/gateway/custody success;
- custody acceptance only after durable relay commit;
- all LoRa TX passes AirtimeManager;
- no required microSD or Internet dependency;
- all queues/tables/retries/custody ownership bounded;
- optional transports/providers removable;
- no custom cryptographic primitives;
- no fabricated hardware evidence;
- no silent feature deletion.

## Completion definition

The coding trigger is complete only when every targeted capability is either implemented with mapped passing evidence for its tier or explicitly blocked by a measured external/hardware/toolchain fact while the stable core remains working.

A feature is not complete merely because source files exist.

## Release handoff

After implementation and applicable evidence gates pass, hand off to `FLASHER_RELEASE_CONTRACT.md` / issue #16. The release pipeline creates the final one-flash package.

## One-click meaning

The goal is one authoritative user action/instruction to start implementation and, later, one supported Flash action for a validated release. It does **not** mean skipping compilation, tests, hardware validation or release gates.
