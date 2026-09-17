# Firmware-mash

Firmware-mash is a research-led, modular off-grid messaging firmware project targeting the LILYGO T-Deck Plus (ESP32-S3 + SX1262) first.

## Mission

Build one coherent communication stack in which a single `HybridRouter` can choose between available links, retain alternate paths, fail over automatically, and queue encrypted messages when no route exists. LoRa remains the mandatory long-range backbone; ESP-NOW Normal/Long Range is planned as an optional local/hybrid lane. NAN, RF-assist/RIS and external backscatter remain experimental until proven useful on real hardware.

A separate `EnergyManager` owns power policy for the T-Deck. It can optimize battery/external-power operation on stock hardware and later accept an optional external Ambient RF Energy Assist provider without making routing dependent on harvesting hardware. Ambient RF harvesting is treated as energy-only and remains LAB until real rectenna/PMIC/storage hardware is measured.

The normal user experience must feel like a compact familiar smartphone: Home, Messages, Contacts, Network and Settings. The user sends a message; routing, retries, LoRa/ESP-NOW choice, multipath, delayed delivery and energy-saving behavior happen automatically in the background.

## Project state

**PRE-BUILD / CODE-READY SUBJECT TO BASELINE EVIDENCE.** No production firmware has been added yet. The `main` branch is the clean release line. Preparatory architecture, licensing, testing, UX, energy-management and implementation contracts live on `develop`.

## Hard rules

- Exactly one logical routing authority: `HybridRouter`.
- Exactly one logical device energy-policy authority: `EnergyManager`.
- LoRa-only operation must always remain possible.
- Core boot, identity, send/receive, queued delivery and recovery must work without a microSD card.
- Stable core operation must work without any RF-harvesting accessory installed.
- A queued message is retried automatically when credible usable connectivity returns; there is no fixed-distance retry trigger.
- One logical PacketId survives retries, transport changes, failover and reboot recovery.
- ESP-NOW Normal and Long Range are capabilities of one transport adapter, not separate user-selected networks.
- Energy harvesting is not a transport and never creates a logical mesh hop.
- No unbounded flooding, queues, routing tables or retries.
- No custom cryptography.
- Experimental features stay behind build flags and cannot silently become stable.
- A compile result is not hardware validation.
- Ambient-RF harvesting cannot be claimed on stock T-Deck hardware without compatible external hardware and measured evidence.
- Upstream code is reused only when licensing is compatible and notices are preserved.
- Missing Firmware-mash integration/policy/provider code is implemented in-project against documented SDK/vendor interfaces rather than silently dropping approved capabilities.
- EU868 duty-cycle and regional limits are treated as design constraints.
- Generated build output and local credentials never belong in Git; dependency lockfiles used for reproducible application builds do.
- Normal users are not the engineering test harness.

## Planned build order

1. Reproduce and pin the selected T-Deck Plus foundation.
2. Prove standalone internal persistence/recovery with no microSD dependency.
3. Add PacketId/event/core primitives while preserving the foundation security/wire envelope.
4. Add EnergyManager with stock battery/external-power abstraction and hysteretic power states.
5. Introduce a LoRa-only HybridRouter skeleton with no behavior regression.
6. Add bounded reliability/dedup and durable delayed delivery.
7. Add bounded multipath route sets, deterministic failover and energy-aware route scoring.
8. Add ESP-NOW Normal + Long Range capability under one optional `TransportAdapter` and validate hybrid forwarding.
9. Integrate the smartphone-like local UI so Messages/Contacts/Network/Settings remain simple while routing and energy policy stay automatic.
10. Add the optional Ambient RF Energy Assist software provider seam/mock without making stable firmware depend on it.
11. Run simulator, host and real multi-node/no-SD/power-loss/re-entry/UI/battery acceptance tests.
12. Produce a one-flash release package only for features that meet their evidence tier.
13. Only then evaluate real Ambient RF harvesting hardware, NAN, RF-assist, TDMA/regional routing and external backscatter.

## One-shot implementation

Once `docs/BASELINE_APPROVED` contains genuine PASS evidence, implementation should follow [`docs/ONE_SHOT_IMPLEMENTATION_RUNBOOK.md`](docs/ONE_SHOT_IMPLEMENTATION_RUNBOOK.md). That runbook defines source-of-truth precedence, coding order, automatic problem-resolution behavior, missing-code implementation rules, controller passes and completion gates so implementation does not repeatedly reopen settled architecture decisions.

## Energy architecture

See [`docs/ENERGY_MANAGEMENT_CONTRACT.md`](docs/ENERGY_MANAGEMENT_CONTRACT.md) and ADR 0006. The software target is:

```text
Battery / USB / board telemetry
            |
            v
       EnergyManager
            |
      EnergyPolicySnapshot
         /         \
        v           v
 HybridRouter    UI/power policy

Optional external RF-harvest hardware
(rectenna/PMIC/storage)
            |
            v
     RFHarvestProvider
            |
            +----> EnergyManager
```

The preferred future RF-harvesting hardware assumption is a separate harvesting antenna/rectenna so the tuned EU868 communications path is not silently loaded or detuned.

## Documentation

Start at [`docs/README.md`](docs/README.md). It indexes architecture, API/wire/delivery contracts, the no-SD storage design, EnergyManager/Ambient RF Energy Assist contract, smartphone UX, build configurations, test traceability, resource budgets, reviews and ADRs.

The intended future code layout is defined in [`docs/REPOSITORY_LAYOUT.md`](docs/REPOSITORY_LAYOUT.md). Production source remains locked by CI until the baseline evidence gate passes.