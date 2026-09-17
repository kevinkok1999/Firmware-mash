# One-Shot Implementation Runbook

## Mission

After the baseline gate is genuinely PASS, implement Firmware-mash from the existing design contracts without reopening settled architecture decisions. The coding agent is expected to solve ordinary implementation problems itself, update tests/evidence, and continue until the planned stage is complete or a blocker is proven by measured hardware/toolchain facts.

## Source of truth order

When implementing, use this precedence:

1. ADRs in `docs/adr/`;
2. `ARCHITECTURE.md`;
3. `PACKET_DELIVERY_CONTRACT.md`;
4. `STANDALONE_TDECK_REQUIREMENTS.md` + `MESSAGE_STORE_DESIGN.md`;
5. `API_CONTRACTS.md`;
6. `ROUTING_SPEC_DRAFT.md`;
7. `BUILD_CONFIG_MATRIX.md`;
8. `UI_UX_CONTRACT.md`;
9. `FEATURE_MANIFEST.md`;
10. `TEST_TRACEABILITY.md`;
11. `IMPLEMENTATION_BLUEPRINT.md`;
12. `RESOURCE_BUDGET.md` and remaining review/test documents.

If two normative documents conflict, do not guess. Resolve the conflict in the smallest possible ADR/doc correction, then continue coding.

## Preconditions

Do not fabricate these. Before production implementation begins, `docs/BASELINE_APPROVED` must contain real PASS evidence for the pinned foundation build/tests/license review/no-SD boot gate.

Baseline evidence must include real flash/RAM figures so capacities and partition sizes can be chosen from measurement rather than assumption.

## Implementation order

Follow `IMPLEMENTATION_BLUEPRINT.md` in order:

1. foundation integration seam;
2. internal durable MessageStore/no-SD;
3. packet/event/core primitives;
4. LoRa transport + AirtimeManager;
5. neighbor + HybridRouter LoRa-only seam;
6. ReliabilityManager + dedup;
7. multipath/failover;
8. ESP-NOW Normal + Long Range capability in one adapter;
9. event-driven delayed delivery;
10. health/metrics/RF intelligence;
11. smartphone-like UI integration;
12. optional lab transports only after stable core.

## Coding behavior

For each component:

1. create the smallest coherent implementation;
2. compile the relevant target/configuration;
3. add/run mapped tests from `TEST_TRACEABILITY.md`;
4. fix failures immediately;
5. record resource impact;
6. verify no architecture boundary was bypassed;
7. continue to the next component only when the current component has bounded failure behavior.

Do not defer solvable compiler/test/design-integration problems to the user.

## Automatic problem-resolution rule

When a problem appears:

```text
observe failure
-> identify owning layer
-> reproduce with smallest test
-> fix root cause
-> run local/component tests
-> run impacted integration tests
-> run CFG-LORA-STABLE regression where applicable
-> document measured resource/behavior change
-> continue
```

Do not paper over a failure by:

- disabling the failing feature silently;
- increasing a retry limit without analysis;
- adding an unbounded queue;
- duplicating a routing engine;
- bypassing AirtimeManager;
- moving required persistent state to microSD;
- marking simulator-only behavior as hardware validated.

## Professional controller pass after every major layer

After Storage, HybridRouter, Multipath, ESP-NOW and UI milestones, perform a red-team review:

- can a failure in this layer break LoRa-only operation?
- are all queues/tables bounded?
- can reboot/power loss corrupt unrelated durable state?
- can one stale event park the state machine forever?
- can duplicate events cause repeated user messages?
- can radio recovery create retry storms?
- did flash/RAM/PSRAM grow beyond the recorded budget?
- is the normal UI still simpler than the internal architecture?

Fix findings before continuing.

## Required configurations

Once code exists, continuously preserve:

- `CFG-LORA-STABLE`;
- `CFG-HYBRID-BETA` once ESP-NOW lands;
- host/unit tests;
- simulator tests.

An optional feature is not isolated unless the build still succeeds with that feature OFF.

## User-experience target

The normal user flow is fixed:

```text
Power on
-> Home
-> Messages
-> contact
-> type
-> Send
```

If unreachable:

```text
Waiting for connection
-> internally queued
-> usable link/route returns later
-> automatic retry
-> Delivered
```

No second Send press and no transport/route engineering by the user.

## ESP-NOW rule

Normal ESP-NOW and ESP-NOW Long Range are capabilities/modes of one adapter. The router uses measured link evidence. The user is not required to select NORMAL versus LR. If ESP-NOW is unavailable or unhealthy, the system falls back to another valid route/LoRa according to policy.

## Completion gate

Implementation is not complete until:

- every STABLE-target item in `FEATURE_MANIFEST.md` is implemented or has a measured blocker;
- all mapped non-hardware tests pass;
- hardware-facing features are labelled according to real evidence;
- the no-SD requirement remains true;
- the one-flash release contract remains achievable;
- resource budgets are measured, not guessed;
- no unresolved architecture contradiction remains.

## What the user should NOT have to do

The user should not be asked to:

- measure packet loss;
- tune routing weights;
- choose next hops;
- debug peer caches;
- manually retry queued messages;
- choose ESP-NOW Normal/LR for each message;
- combine separate firmware binaries;
- repair ordinary implementation bugs.

Those are implementation/release responsibilities.