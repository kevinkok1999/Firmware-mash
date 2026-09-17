# Repository Layout Contract

This file defines where Firmware-mash source belongs after baseline approval.

## Baseline state

Production source was locked until `docs/BASELINE_APPROVED` existed with real evidence. The baseline is now approved, so production components may be added only in the ownership boundaries below.

## Foundation strategy

The selected Bramble foundation remains pinned and coherent. Do not cosmetically copy/rename the full upstream tree into parallel Firmware-mash implementations. Firmware-mash-owned policy, adapters and hardening code use `mog_` components and are integrated over the pinned foundation according to ADR 0009.

## Target logical layout

```text
components/
  mog_core/
  mog_packet/
  mog_events/
  mog_conversation/
  mog_messaging/

  mog_message_store/
  mog_custody/

  mog_neighbor/
  mog_routing/
  mog_route_discovery/
  mog_multipath/
  mog_route_score/
  mog_path_diversity/

  mog_reliability/
  mog_dedup/

  mog_transport_lora/
  mog_transport_espnow/
  mog_transport_ip/
  mog_transport_nan/          # LAB only until promoted

  mog_gateway/
  mog_airtime/
  mog_radio_scheduler/
  mog_rf_intelligence/

  mog_energy/
  mog_energy_rf_harvest/
  mog_rf_assist/

  mog_health/
  mog_metrics/
  mog_update/

main/
  target/application integration only

test/
  host/unit/fault-injection tests

simulator/
  deterministic network, energy-policy and hardware-provider scenarios

docs/
  architecture, decisions, evidence and operating documentation

scripts/
  foundation sync, phase gates, build/release/package automation

tools/
  analysis/development utilities that are not target firmware
```

## Ownership rules

- `main/` remains thin; reusable routing/storage/transport/energy/conversation logic belongs in components.
- `HybridRouter` in `mog_routing` is the only logical routing authority.
- `EnergyManager` in `mog_energy` is the only device energy-policy authority.
- `AirtimeManager`/the approved airtime adapter is the only authority that may authorize LoRa airtime.
- Conversation identity is independent of transport and route; transport adapters may not create chats.
- `mog_custody` owns custody responsibility state but may not mark user-visible delivery; only destination E2E ACK can do that.
- `mog_gateway` discovers/scores/maintains gateway capability and federation state; it does not become a second router.
- `mog_transport_ip` owns IP framing/session transport and accepts Wi-Fi/cellular NetifProviders; it does not own message identity or delivery truth.
- Hardware callbacks live in adapter/provider/board layers and never mutate routing state directly.
- Energy-source providers report measurements/capabilities only; they never own routing or delivery state.
- `test/` and `simulator/` use production components; do not create shadow protocol/routing implementations for tests.
- LAB transports/providers stay behind build flags and cannot become dependencies of the stable LoRa-only core.
- Board-specific pins/config remain in the foundation board/config layer; do not scatter GPIO constants across components.
- Generated build output, credentials, signing keys and local SDK state never belong in Git.

## Source naming

Firmware-mash-owned reusable components use the `mog_` prefix until an ADR deliberately changes the namespace. Imported upstream code retains original notices/naming unless a documented integration patch is necessary.

If required integration/provider code does not exist upstream, implement original `mog_` code against documented vendor/SDK interfaces rather than silently dropping the capability. Do not create custom cryptographic primitives.

## Dependency direction

Preferred direction:

```text
UI/Application
    -> Conversation/Messaging/Security/Reliability
    -> HybridRouter <---- EnergyPolicySnapshot
    -> Transport interfaces
       -> LoRa adapter -> AirtimeManager -> SX1262
       -> ESP-NOW adapter -> RadioScheduler -> ESP32 radio
       -> IP adapter -> Wi-Fi/Cellular NetifProvider
    -> Hardware drivers

GatewayManager -------> capability/path input to HybridRouter
CustodyManager -------> durable responsibility state, never delivery truth
Board/PMIC telemetry -> EnergyManager
Optional RF harvest -> EnergyManager
```

Lower layers do not call upward into UI or make user-facing routing decisions.

## Storage placement

- identity/keys/configuration: dedicated internal durable storage;
- pending/outgoing/custody durable messages: bounded internal flash MessageStore;
- routes/neighbors/RF/energy history: RAM/PSRAM by default;
- microSD: optional extension/export only, never required for core boot, send, receive, failover or recovery;
- high-frequency energy/harvest samples: transient unless a deliberately bounded diagnostic export is requested.

## Change rule

A new top-level source directory or a second owner for routing, energy, airtime, message identity or delivery truth requires an ADR/architecture review. This prevents the repository from drifting into parallel implementations.