# Coding Trigger Contract

## Purpose

Provide one authoritative entrypoint for the future production implementation pass. The trigger is a contract for a coding agent, not permission to bypass the baseline gate.

## Construction principle: build it like a house

Firmware-mash is implemented bottom-up. A higher layer is not allowed to become the foundation for a lower layer that has not yet been proven.

The rule is:

```text
survey/evidence
-> foundation
-> utilities/structural core
-> communication structure
-> reliability/safety systems
-> optional extensions
-> user-facing rooms/UI
-> inspection
-> release/flasher
```

Every stage must carry its own load before the next stage depends on it. If a foundation-stage test fails, fix that layer first; do not hide the defect by adding logic higher in the stack.

### House-to-firmware mapping

```text
Ground survey       = pinned Bramble/T-Deck baseline + measured resources
Foundation          = board integration + internal durable MessageStore
Concrete/core       = PacketId + events + lifecycle + reboot-safe time/identity
Utilities           = EnergyManager + health primitives
Structural frame    = LoRa transport + AirtimeManager + HybridRouter
Safety systems      = ReliabilityManager + ACK/retry + dedup + delayed delivery
Extra structure     = multipath + custody/store-carry-forward
Extensions          = ESP-NOW/LR + IP/gateway/cellular providers
Rooms/controls      = smartphone-like UI + contacts/messages/settings
Optional annex      = RF-harvest seam + LAB transports
Inspection          = controller/security/resource/hardware evidence gates
Handover            = reproducible release package + one-click flasher + recovery
```

No optional extension may become necessary for the foundation/stable core.

## Required trigger condition

Production implementation starts only when:

- branch is `develop` or an explicitly derived implementation branch;
- `docs/BASELINE_APPROVED` exists and passes preflight marker checks;
- the final controller report has no unresolved architecture contradiction;
- required normative docs/ADRs are present;
- pinned foundation/toolchain/provenance data are available.

If these conditions are not satisfied, the trigger stops before creating production source.

## Authoritative instruction

> Execute Firmware-mash from the current repository contracts using a strict foundation-first construction strategy. Do not redesign settled architecture. Follow `ONE_SHOT_IMPLEMENTATION_RUNBOOK.md` and `IMPLEMENTATION_BLUEPRINT.md` in dependency order. Never build a higher-level feature to compensate for an unproven lower-level dependency. Implement missing approved `mog_` code rather than omitting capabilities. After each major layer, compile, run mapped tests, run the LoRa-stable regression, record resource/evidence impact, repair ordinary failures in-loop, and continue. Stop only for a proven measurement/hardware/toolchain/access blocker. Never invent hardware validation, custom crypto or release evidence.

## Document precedence

1. accepted ADRs;
2. `ARCHITECTURE.md`;
3. packet/delivery/custody/IP/energy/storage/runtime contracts;
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
3. packet/event/core primitives, including reboot-safe PacketId/time semantics;
4. EnergyManager + core health/runtime-safety primitives;
5. LoRa + AirtimeManager;
6. HybridRouter LoRa-only seam;
7. ReliabilityManager + dedup + sender delayed delivery;
8. bounded multipath/route scoring;
9. optional custody core/store-carry state machine on the proven LoRa/storage/reliability foundation;
10. ESP-NOW Normal/LR adapter + 2.4-GHz RadioScheduler;
11. IP backhaul + gateway federation;
12. cross-transport recovery/event integration for queued/custody-held messages;
13. health/metrics/controller checks;
14. smartphone-like UI integration;
15. optional Ambient RF provider seam;
16. optional lab transports/hardware providers;
17. release-engineering handoff.

Why custody precedes extra transports: its durability/ownership semantics must first be proven without mixing in Wi-Fi/ESP-NOW complexity. Later transports simply become additional forwarding opportunities for the same custody-held PacketId.

## Stage admission rule

Before starting stage N+1, stage N must have:

- compiling target/configuration;
- mapped tests implemented and passing at the evidence level currently possible;
- bounded failure behavior;
- no new architecture ownership violation;
- recorded flash/RAM/PSRAM/persistence impact where measurable;
- CFG-LORA-STABLE regression passing whenever that configuration already exists;
- no unresolved defect that the next stage would merely mask.

Hardware-only acceptance may remain pending where no physical setup exists, but that uncertainty must remain explicit and may not be relabelled as PASS.

## Per-stage loop

```text
implement smallest coherent layer
-> compile target/config
-> run mapped unit/simulator tests
-> run impacted integration tests
-> run CFG-LORA-STABLE regression
-> inspect resource delta
-> inspect architecture/security boundary
-> repair failure at its owning layer
-> record evidence
-> admit next layer
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
- no silent feature deletion;
- no higher-layer workaround for an unresolved lower-layer defect.

## Completion definition

The coding trigger is complete only when every targeted capability is either implemented with mapped passing evidence for its tier or explicitly blocked by a measured external/hardware/toolchain fact while the stable core remains working.

A feature is not complete merely because source files exist.

## Release handoff

After implementation and applicable evidence gates pass, hand off to `FLASHER_RELEASE_CONTRACT.md` / issue #16. The release pipeline creates the final one-flash package.

## One-click meaning

The goal is one authoritative user action/instruction to start implementation and, later, one supported Flash action for a validated release. It does **not** mean skipping compilation, tests, hardware validation or release gates.
