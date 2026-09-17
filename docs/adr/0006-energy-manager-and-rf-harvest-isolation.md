# ADR 0006 — EnergyManager and RF-harvest isolation

## Status

Accepted for implementation planning.

## Context

Firmware-mash needs aggressive handheld power management and may later use external ambient-RF energy harvesting hardware. A stock T-Deck Plus cannot gain RF energy harvesting through firmware alone, and tying power electronics directly to routing would create unsafe coupling, difficult testing and false product claims.

## Decision

1. Introduce exactly one logical `EnergyManager` as the device energy-policy authority.
2. Battery/USB/board power telemetry is part of the normal software architecture.
3. External ambient-RF harvesting is represented by an optional provider below EnergyManager, not by a transport and not by HybridRouter.
4. RF harvesting is energy-only; no decoding of third-party traffic is required.
5. HybridRouter receives normalized energy cost/budgets only and never drives PMIC/rectifier/supercap hardware directly.
6. CFG-LORA-STABLE must compile and operate with every harvesting provider removed.
7. Preferred physical design uses a separate harvesting antenna/rectenna. Shared-antenna designs require measured RF isolation/insertion-loss/desense evidence before enablement.
8. Exact battery, PMIC and TX-reservoir thresholds are frozen only from measured hardware data and use hysteresis.
9. Ambient-RF harvesting remains LAB until real hardware demonstrates repeatable useful net energy without unacceptable communication, safety or compliance regression.

## Consequences

### Positive

- Power policy is reusable even without harvesting hardware.
- Routing remains deterministic and testable.
- Stock T-Deck behavior cannot become dependent on experimental energy hardware.
- Future solar/thermal/other harvesters can reuse the provider model.
- Product claims remain evidence-based.

### Negative

- Additional state/policy/test surface.
- True RF harvesting still requires external hardware and hardware validation.
- Energy-aware route scoring must be carefully tuned so power savings do not reduce delivery reliability.

## Rejected alternatives

### Treat RF harvesting as a transport

Rejected because harvested RF energy does not carry a Firmware-mash logical packet and must not become a routing hop.

### Let HybridRouter control harvesting hardware

Rejected because it creates cross-layer coupling and makes both routing and power failures harder to isolate.

### Share the LoRa antenna by default

Rejected because an unvalidated harvesting load/switch can detune or desensitize the communications RF path.

### Make stable firmware depend on harvested energy

Rejected because ambient energy is variable and absent in many environments.

## Verification

See `ENERGY_MANAGEMENT_CONTRACT.md` and `TEST_TRACEABILITY.md`. The release gate requires that harvesting OFF remains a complete working configuration and that any real RF-harvest implementation has measured RF/power evidence before promotion.