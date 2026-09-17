# One-Shot Implementation Runbook

## Mission

After the baseline gate is genuinely PASS, implement Firmware-mash from the existing design contracts without reopening settled architecture decisions. The coding agent is expected to solve ordinary implementation problems itself, update tests/evidence, and continue until the planned stage is complete or a blocker is proven by measured hardware/toolchain facts.

## Source of truth order

When implementing, use this precedence:

1. ADRs in `docs/adr/`;
2. `ARCHITECTURE.md`;
3. `PACKET_DELIVERY_CONTRACT.md`;
4. `STANDALONE_TDECK_REQUIREMENTS.md` + `MESSAGE_STORE_DESIGN.md`;
5. `ENERGY_MANAGEMENT_CONTRACT.md`;
6. `IP_GATEWAY_FEDERATION_CONTRACT.md`;
7. `API_CONTRACTS.md`;
8. `ROUTING_SPEC_DRAFT.md`;
9. `BUILD_CONFIG_MATRIX.md`;
10. `UI_UX_CONTRACT.md`;
11. `FEATURE_MANIFEST.md`;
12. `TEST_TRACEABILITY.md`;
13. `IMPLEMENTATION_BLUEPRINT.md`;
14. `RESOURCE_BUDGET.md` and remaining review/test documents.

If two normative documents conflict, do not guess. Resolve the conflict in the smallest possible ADR/doc correction, then continue coding.

## Preconditions

Do not fabricate these. Before production implementation begins, `docs/BASELINE_APPROVED` must contain real PASS evidence for the pinned foundation build/tests/license review/no-SD dependency gate.

Baseline evidence must include real flash/RAM figures so capacities and partition sizes can be chosen from measurement rather than assumption.

## Implementation order

Follow `IMPLEMENTATION_BLUEPRINT.md` in order:

1. foundation integration seam;
2. internal durable MessageStore/no-SD;
3. packet/event/core primitives;
4. EnergyManager foundation with battery/external-power abstraction;
5. LoRa transport + AirtimeManager;
6. neighbor + HybridRouter LoRa-only seam;
7. ReliabilityManager + dedup;
8. multipath/failover + energy-aware route scoring;
9. ESP-NOW Normal + Long Range capability in one adapter;
10. IP backhaul + GatewayManager/GatewayDiscovery + Wi-Fi provider;
11. event-driven delayed delivery across radio/IP recovery;
12. health/metrics/RF intelligence/energy/gateway metrics;
13. smartphone-like UI integration;
14. optional Ambient RF Energy Assist provider seam/mock;
15. optional lab transports/advanced federation only after stable core.

Cellular is implemented inside the IP-backhaul phase only after a specific compatible modem target is selected; compile-only generic cellular claims are not treated as hardware support.

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

## Missing-code rule

If required code does not already exist, implement it rather than silently omitting the feature.

This includes Firmware-mash-owned:

```text
state machines
policy engines
driver shims
ESP-IDF/vendor integration glue
simulator models
EnergyManager providers
IP NetifProviders
mog_transport_ip
GatewayManager / GatewayDiscovery
federation framing/session logic
mock/unavailable hardware providers
UI bindings
health/metrics plumbing
```

Implementation rules:

- use documented SDK/vendor/datasheet interfaces;
- prefer wrapping maintained vendor drivers over unnecessary forks;
- write original project code when no compatible implementation exists;
- preserve provenance/licenses for reused code;
- reject incompatible-license copying;
- write tests for new policy/state-machine logic;
- provide safe hardware-absent behavior for optional providers;
- do not invent custom cryptographic primitives.

A genuinely missing hardware capability may remain evidence-gated, but its software boundary/mock must still be implemented when it is in the approved plan.

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
- duplicating an energy-policy engine;
- treating GatewayManager as a second routing authority;
- resetting PacketId/chat because a message crosses IP;
- equating socket/TLS write success with end-to-end delivery;
- bypassing AirtimeManager;
- moving required persistent state to microSD/cloud;
- making one cloud server mandatory for local messaging;
- marking simulator-only behavior as hardware validated;
- claiming ambient-RF harvesting without compatible measured hardware.

## Professional controller pass after every major layer

After Storage, EnergyManager, HybridRouter, Multipath, ESP-NOW, IP/Gateway and UI milestones, perform a red-team review:

- can a failure in this layer break LoRa-only operation?
- are all queues/tables/sessions bounded?
- can reboot/power loss corrupt unrelated durable state?
- can one stale event park the state machine forever?
- can duplicate events cause repeated user messages?
- can radio, gateway or energy recovery create retry storms?
- can energy-state noise cause route/power-mode flapping?
- can an optional harvester provider fail without affecting stock T-Deck operation?
- can Internet/bootstrap loss split the chat or erase local message history?
- can unauthenticated/malformed gateway data consume unbounded resources?
- does public federation avoid requiring plaintext user-message content?
- did flash/RAM/PSRAM grow beyond the recorded budget?
- is the normal UI still simpler than the internal architecture?

Fix findings before continuing.

## Required configurations

Once code exists, continuously preserve:

- `CFG-LORA-STABLE`;
- `CFG-HYBRID-BETA` once ESP-NOW lands;
- `CFG-IP-BETA` once IP backhaul lands;
- `CFG-GATEWAY-BETA` once federation/gateway listener code exists;
- `CFG-ENERGY-LAB` once the harvesting-provider seam exists;
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

The same contact stays in one conversation even when one message uses LoRa and the next uses IP/gateway federation.

If unreachable:

```text
Waiting for connection
-> internally queued
-> usable radio/IP/gateway route returns later
-> automatic retry
-> Delivered
```

No second Send press and no per-message transport/route/gateway engineering by the user.

Energy-saving states are automatic. Normal users do not manage rectifier, MPPT, supercapacitor or route-energy parameters.

## ESP-NOW rule

Normal ESP-NOW and ESP-NOW Long Range are capabilities/modes of one adapter. The router uses measured link evidence. The user is not required to select NORMAL versus LR. If ESP-NOW is unavailable or unhealthy, the system falls back to another valid route/LoRa according to policy.

## IP/gateway rule

Wi-Fi and cellular are bearer providers of one `mog_transport_ip`. GatewayManager/GatewayDiscovery provide bounded authenticated reachability evidence; `HybridRouter` remains the routing authority.

Normal Wi-Fi access points are IP access media, not unconfigured Firmware-mash relays. Cellular requires actual supported modem/network access. Handhelds use outbound authenticated sessions by default; public inbound listeners are gateway-class functionality.

The initial federation must support more than one configured/learned peer and must continue using already-established peers if a bootstrap endpoint disappears. Loss of all Internet paths falls back to radio/store-forward rather than breaking the conversation model.

## Energy-management rule

`EnergyManager` is the only device energy-policy authority. It publishes normalized budgets/snapshots; routing and transports do not read raw PMIC/harvester state directly.

The stable build must operate normally with RF harvesting absent/disabled. Ambient RF Energy Assist remains LAB until real external hardware demonstrates repeatable useful net energy and acceptable RF coexistence.

## Completion gate

Implementation is not complete until:

- every STABLE-target item in `FEATURE_MANIFEST.md` is implemented or has a measured blocker;
- all mapped non-hardware tests pass;
- hardware-facing features are labelled according to real evidence;
- the no-SD requirement remains true;
- EnergyManager works without RF-harvest hardware;
- optional harvesting code can be compiled out without regression;
- IP/gateway code can be compiled out while CFG-LORA-STABLE still works;
- IP beta preserves the same PacketId/chat across route changes;
- gateway/bootstrap failure has bounded recovery behavior;
- cellular claims are made only for real selected hardware evidence;
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
- choose LoRa/Wi-Fi/cellular/gateway per message;
- maintain separate chats for Internet/off-grid delivery;
- configure federation peer tables for ordinary use;
- tune energy-state thresholds;
- configure harvester electronics for normal stable operation;
- combine separate firmware binaries;
- repair ordinary implementation bugs.

Those are implementation/release responsibilities.
