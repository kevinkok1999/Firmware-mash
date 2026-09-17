# Repository Layout Contract

This file defines where future Firmware-mash source belongs after the baseline gate unlocks production code.

## Before baseline approval

Allowed top-level project content is intentionally small:

```text
.github/
docs/
.editorconfig
.gitignore
CONTRIBUTING.md
LICENSE
README.md
SECURITY.md
```

Production firmware, copied upstream source, simulator source and experimental code must not be added to this repository before `docs/BASELINE_APPROVED` exists with real PASS evidence.

## After baseline approval

The selected foundation may require retaining some upstream directory names. Prefer preserving upstream coherence instead of performing a cosmetic mass-rename. New Firmware-mash-specific code should nevertheless follow clear ownership boundaries.

Target logical layout:

```text
components/
  mog_core/
  mog_packet/
  mog_events/
  mog_neighbor/
  mog_routing/
  mog_route_discovery/
  mog_multipath/
  mog_route_score/
  mog_path_diversity/
  mog_reliability/
  mog_dedup/
  mog_message_store/
  mog_transport_lora/
  mog_transport_espnow/
  mog_transport_nan/
  mog_airtime/
  mog_radio_scheduler/
  mog_rf_intelligence/
  mog_energy/
  mog_energy_rf_harvest/
  mog_rf_assist/
  mog_health/
  mog_metrics/

main/
  target/application integration only

test/
  host/unit tests

simulator/
  deterministic network, energy-policy and hardware-provider scenarios

docs/
  architecture, decisions, evidence and operating documentation

tools/
  build/release/analysis utilities that are not target firmware
```

## Ownership rules

- `main/` must remain thin; reusable routing/storage/transport/energy logic belongs in components.
- Hardware callbacks live in their adapter/provider/board layer and must not mutate routing state directly.
- `HybridRouter` is the only logical routing authority.
- `EnergyManager` is the only logical device energy-policy authority.
- Energy-source providers only report measurements/capabilities; they do not own routing or delivery state.
- `test/` and `simulator/` may use the same production components; do not create separate shadow protocol or policy implementations.
- Lab transports/providers remain isolated behind build flags and may not become dependencies of the stable LoRa-only core.
- Board-specific pin definitions/config stay in the selected foundation's board/config layer; do not scatter GPIO constants across components.
- Generated build output, credentials, signing keys and local SDK state never belong in Git.

## Source naming

Firmware-mash-owned reusable components use the `mog_` prefix until a later ADR deliberately changes the namespace. Imported upstream code retains its original notices and naming unless modification is necessary and provenance is recorded.

If needed integration/provider code does not exist upstream, implement original `mog_` code against documented vendor/SDK interfaces rather than silently dropping the feature. Do not create custom cryptographic primitives.

## Dependency direction

Preferred direction:

```text
UI/Application
    -> Messaging/Security/Reliability
    -> HybridRouter <---- EnergyPolicySnapshot
    -> Transport interfaces
    -> Hardware drivers

Board/PMIC telemetry ---> EnergyManager
Optional RF harvest HW -> EnergyManager
```

Lower layers must not call upward into UI or make user-facing routing decisions. Hardware providers must not modify HybridRouter state directly.

## Storage placement

- identity/keys/configuration: dedicated internal durable storage;
- pending durable messages: bounded internal flash MessageStore;
- routes/neighbors/RF/energy history: RAM/PSRAM by default;
- microSD: optional extension only, never required for core boot, send, receive, failover or recovery;
- high-frequency energy/harvest samples: transient only unless a deliberately bounded diagnostic export is requested.

## Change rule

A new top-level source directory requires an ADR or explicit architecture review. This prevents the repository from slowly becoming a collection of parallel implementations.