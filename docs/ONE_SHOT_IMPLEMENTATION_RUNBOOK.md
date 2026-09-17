# One-Shot Implementation Runbook

## Mission

After the baseline gate is genuinely PASS, implement Firmware-mash from the settled contracts without reopening architecture decisions. Ordinary compiler/integration/test problems are solved in-loop; only proven hardware/toolchain/access blockers may stop progress.

## Source of truth order

1. accepted ADRs in `docs/adr/`;
2. `ARCHITECTURE.md`;
3. `PACKET_DELIVERY_CONTRACT.md` + `STORE_CARRY_CUSTODY_CONTRACT.md`;
4. `STANDALONE_TDECK_REQUIREMENTS.md` + `MESSAGE_STORE_DESIGN.md`;
5. `ENERGY_MANAGEMENT_CONTRACT.md`;
6. `IP_GATEWAY_FEDERATION_CONTRACT.md` + `GATEWAY_SECURITY_REQUIREMENTS.md`;
7. `API_CONTRACTS.md`;
8. `ROUTING_SPEC_DRAFT.md`;
9. `BUILD_CONFIG_MATRIX.md`;
10. `UI_UX_CONTRACT.md`;
11. `FEATURE_MANIFEST.md`;
12. `TEST_TRACEABILITY.md`;
13. `IMPLEMENTATION_BLUEPRINT.md`;
14. `FLASHER_RELEASE_CONTRACT.md`;
15. resource/review/evidence documents.

If normative files conflict, fix the owning ADR/contract first; do not improvise around it in code.

## Preconditions

Before production source begins:

- `docs/BASELINE_APPROVED` must contain real valid PASS evidence;
- baseline flash/RAM/PSRAM evidence must exist;
- final controller review must show no unresolved architecture contradiction;
- `CODING_TRIGGER_CONTRACT.md` conditions must pass.

## Implementation order

1. foundation integration seam;
2. internal durable MessageStore/no-SD;
3. packet/event/core primitives;
4. EnergyManager;
5. LoRa + AirtimeManager;
6. neighbor + HybridRouter LoRa-only seam;
7. ReliabilityManager + dedup + sender delayed delivery;
8. multipath/failover + route scoring;
9. optional custody core/store-carry-forward on LoRa-first foundation;
10. ESP-NOW Normal/LR;
11. IP backhaul + GatewayManager/Discovery + Wi-Fi provider;
12. cross-transport queued/custody recovery events;
13. health/metrics/controller pass;
14. smartphone-like UI;
15. optional Ambient RF Energy Assist provider seam/mock;
16. optional lab transports/providers;
17. release-engineering handoff.

Cellular is only implemented/advertised against selected compatible modem hardware. Custody core is proven before optional transports so ownership/durability bugs are not confused with transport bugs.

## Per-component loop

```text
implement smallest coherent unit
-> compile relevant config
-> run mapped unit/simulator tests
-> run impacted integration tests
-> run CFG-LORA-STABLE regression
-> inspect flash/RAM/PSRAM/resource delta
-> inspect security/architecture boundaries
-> fix root cause of failures
-> record evidence tier
-> continue
```

Do not defer ordinary implementation problems to the user.

## Missing-code rule

If required code does not exist, implement it rather than silently omitting the capability. This includes state machines, policy engines, driver shims, ESP-IDF/vendor glue, MessageStore/custody logic, simulator models, EnergyManager providers, IP NetifProviders, `mog_transport_ip`, GatewayManager/Discovery, federation/session logic, UI bindings and health metrics.

Use documented APIs, preserve licenses/provenance and never invent custom cryptographic primitives.

## Automatic problem-resolution rule

When a problem appears:

```text
observe
-> identify owning layer
-> reproduce minimally
-> fix root cause
-> run local tests
-> run impacted integration tests
-> run stable regression
-> document measured impact
-> continue
```

Forbidden shortcuts include:

- silently disabling a failing approved feature;
- raising retry limits without analysis;
- adding unbounded queues/tables/replication;
- duplicating routing, energy or delivery authority;
- treating custody acceptance as Delivered;
- accepting custody before durable commit;
- treating socket/TLS write as Delivered;
- resetting PacketId/chat when crossing IP/custody;
- bypassing AirtimeManager;
- moving required durable state to microSD/cloud;
- making one cloud/bootstrap server mandatory;
- labelling simulator success as hardware validation;
- claiming RF harvesting/cellular without matching hardware evidence.

## Controller pass after every major layer

After Storage, EnergyManager, HybridRouter, Reliability, Multipath, Custody, ESP-NOW, IP/Gateway and UI milestones verify:

- LoRa-only operation still works;
- all queues/tables/sessions/custody records are bounded;
- reboot/power loss cannot corrupt unrelated durable state;
- stale events cannot park the state machine forever;
- duplicates cannot create repeated chat messages;
- route/gateway/energy/custody recovery cannot create storms;
- custody cannot acknowledge responsibility before durable commit;
- optional provider/transport failure cannot break stable core;
- Internet/bootstrap loss cannot split the chat or erase local history;
- malformed/unauthenticated Internet/custody input fails closed;
- flash/RAM/PSRAM remains inside measured budgets;
- normal UI remains simpler than internal architecture.

Fix findings before continuing.

## Required configurations

Continuously preserve as applicable:

- `CFG-LORA-STABLE`;
- `CFG-HYBRID-BETA`;
- `CFG-CUSTODY-BETA`;
- `CFG-IP-BETA`;
- `CFG-GATEWAY-BETA`;
- `CFG-ENERGY-LAB`;
- host/unit tests;
- simulator tests.

An optional feature is not isolated unless the same commit builds with it OFF and stable sender delayed delivery still works.

## User-experience target

```text
Power on -> Home -> Messages -> contact -> type -> Send
```

The same contact stays one conversation regardless of LoRa/ESP-NOW/IP/custody path.

If unreachable:

```text
Waiting for connection
-> durable queue/custody policy internally
-> usable path later returns
-> automatic retry/forward
-> destination E2E ACK
-> Delivered
```

No second Send press and no per-message route engineering.

## Transport/custody rules

- ESP-NOW Normal/LR are modes of one adapter.
- Wi-Fi/cellular are bearers of one `mog_transport_ip`.
- GatewayManager provides reachability evidence, not route ownership.
- custody provides bounded durable ownership/reconciliation, not route selection or delivery truth.
- sender WAITING_ROUTE remains available when custody is disabled.

## Energy-management rule

EnergyManager alone owns device power policy. Stable firmware works without RF harvesting. Ambient RF stays LAB until measured compatible hardware evidence exists.

## Completion gate

Implementation is not complete until:

- every STABLE-target item is implemented or has a measured blocker;
- all mapped non-hardware tests pass;
- hardware-facing features are labelled only at real evidence tier;
- no-SD remains true;
- stable core works with harvesting/IP/custody optional features OFF;
- PacketId/chat identity survive every enabled route/transport/custody transition;
- gateway/bootstrap and custody failure behavior is bounded;
- one-flash release remains achievable;
- resource budgets are measured;
- no architecture contradiction remains.

## Release handoff

When the selected tier passes evidence, follow `FLASHER_RELEASE_CONTRACT.md` and issue #16. Generate build artifacts/manifest/hashes/flasher descriptor from real partition/build metadata; never ask normal users to combine binaries manually.

## What the user should not have to do

The user should not be asked to tune routes, next hops, retry counts, peer caches, custody holders, gateway peers, ESP-NOW mode, Wi-Fi/cellular route, energy thresholds, harvester electronics, binary offsets or recovery internals, nor to repair ordinary implementation bugs.
